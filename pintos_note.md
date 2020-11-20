#                  					Pintos 笔记

 

## Part one. Alarm Clock

### 问题

出现忙等待（“busy wait"),出现空转现象。这回使得这个进程一直空转来检查当前时间并调用`thread_yield()`函数直到时间片结束。

### 目标

让它避免忙等待

### 模块

#### 1. timer_sleep

##### 先调用`timer_ticks()` 

###### timer_ticks()

```c

timer_ticks (void)
 {
   enum intr_level old_level = intr_disable ();
   int64_t t = ticks;
   intr_set_level (old_level);
   return t;
 }
 
```

 第一行的过程：`intr_disable()` 调用 `intr_get_level()` ，`intr_get_level()`   返回intr_level的值，这个值表示当前终端的状态。`intr_get_level()` 函数中调用了汇编的语句，该语句的意思是当前终端的状态是否能被中断，flag=0表示不可被中断。`intr_disable()` 通过调用`intr_get_level()` 来获取了当前的中断状态。然后再通过汇编语句使当前状态不能被中断。并把此状态前的中断状态保存在`intr_level` 中，之前的中断状态保存在`old_level` 中。**总结** ：禁止当前行为被中断，保存之前的中断状态。

第三行的过程：将保存的之前的中断状态作为参数传递给`intr_set_level()` ,`intr_set_level()` 再用判别式来设置当前IF的值，如果`old_level` 是`intr_on` ,就将当前状态设置为可以被中断，否则不可以。（前提是当前外中断标识位是0，即无外中断。） **总结**  ：即根据之前的中断状态恢复现场（个人理解）。 

**`timer_ticks()`** 总结：先让当前状态不可被中断，并保存之前的中断状态。在设置完当前中断状态后，根据保存的之前中断状态的信息回复之前的中断状态。

**总结的总结：** 这个`timer_ticks()` 函数是保证获取`ticks` 的值，并且保证这一操作是原子性的。

##### `ticks`的含义：

`static int64_t ticks;`
从pintos被启动开始， `ticks`就一直在计时， 代表着操作系统执行单位时间的前进计量。

#### `timer_sleep()`函数

```c
void timer_sleep(int64_t ticks){
  init64_t start = timer_ticks();
  ASSERT (intr_get_level)() == INTR_ON;//当前状态能被中断才能进行下列语句
  while(timer_elapsed (start) < ticks){
    thread_yield ();
  }
}
```

第一行 `start` 通过调用函数`timer_ticks()` 获取了进程起始时间。

第三行跳出while的条件：在x个时间片中一直进行`thread_yield` 直到进行了x个时间片。(x取决于测试文件分配多少个给到这个函数)。

**`thread_yield()`:** 

```C
void
thread_yield (void) 
{
  struct thread *cur = thread_current ();
  enum intr_level old_level;
  ASSERT (!intr_context ());//断言当前是软中断
  old_level = intr_disable ();//保证这个操作是原子操作
  if (cur != idle_thread) 
    list_push_back (&ready_list, &cur->elem);
  cur->status = THREAD_READY;
  schedule ();
  intr_set_level (old_level);
}
```

 

第一行调用`thread_current()` ,`thread_current()` 再调用`running_thread()` 。`running_thread()` 先通过汇编语言将存储线程的栈指针赋值给`esp` ，但是目标`struct thread` 永远在最开头，所要为了保险，要调用`pg_round_down(esp)` 定位到最开头的位置，并将这个位置赋值给`cur` 。

第五行及往下:判断当前线程是不是空闲线程，如果不是，就push进就绪队列中。把当前线程的状态改成`THREAD_READY` 。最后调用`schedule()` 。

**`schedule()` **：

```c
schedule (void) 
{
  struct thread *cur = running_thread ();//将当前线程的“首地址”赋值给cur
  struct thread *next = next_thread_to_run ();
  struct thread *prev = NULL;

  ASSERT (intr_get_level () == INTR_OFF);
  ASSERT (cur->status != THREAD_RUNNING);
  ASSERT (is_thread (next));

  if (cur != next)
    prev = switch_threads (cur, next);
  thread_schedule_tail (prev);
}
```

`next` 通过调用函数`next_thread_to_run` 得到一个就绪队列中第一个线程的指针。(如果这个队列是一个空队列，就会得到`idle_thread` )。

接下来三个断言：当前线程是不可被中断的&&当前线程不在运行（运行完了？）&&下个线程不是空。

如果下一个线程和当前线程不同，就进行`switch_threads()` ,进行线程间的切换。在`switch_threads()` 这个函数实现中，用到了汇编语言。在切换的时候保留“ebx,ebp,esi,edi"这四个寄存器。

**总结:**  `schedule`先把当前线程丢到就绪队列，然后把线程切换如果下一个线程和当前线程不一样的话。

最后一行：

`thread_schedule_tail(prev)` :先获得当前线程cur,并把状态改成running，然后把时间片清零。然后更新页目录表并且更新任务现场信息（TSS）。然后进行判断，如果当前进程是一个没用的进程，就清空资源。

***总结`thread_yield()`:***   把当前线程扔到就绪队列里，然后重新进行调度，如过就绪队列为空那么就不会切换线程，当前线程继续执行，否则就切换到下一个线程。

### 总结timer_sleep()函数

功能是不断的把当前进程放到就绪队列里，然后重新进行调度，如果就绪队列有东西就会切换到下一个线程。

问题：就绪队列的下一个线程就是他自己。所以形成了一个大循环就是当前线程不断切到自己身上。这样系统就会不让当前线程执行它该有的操作，以此来达到“sleep”的效果。

### 解决方案

换一种让线程等待的方式。`ticks` 表示要等待多久，所以当`ticks` 减为0的时候就代表线程结束等待。所以给当前线程加一个记录要等待多久的一个属性，然后当`ticks` 不为0的时候`block()` 线程，为0时进行`unblock()` 操作即可。



## PART TWO.Priority Scheduling

### 优先级定义

在Pintos中实现优先级调度。当一个线程被添加到比当前运行的线程优先级更高的就绪列表中时，当前线程应该立即将处理器分配给新线程。类似地，当线程在等待锁、信号量或条件变量时，应该首先唤醒优先级最高的等待线程。一个线程可以在任何时候提高或降低它自己的优先级，但是降低它的优先级以使它不再具有最高优先级，必须使它立即让出CPU。线程优先级从`PRI_MIN(0)`到`PRI_MAX(63)`。线程优先级是创建线程是传给`thread_create()`的参数，默认值为`PRI_DEFAULT(31)`。

### 问题

高级别的线程有时候因为一些原因永远不能得到CPU的使用权。这个问题的解决方法之一就是让高优先级的线程将自己的级别“捐赠”给低优先级别的线程，然后让原本低优先级的线程运行，在占据CPU的低优先级线程完成后被释放了，就再让高优先级的线程拿回属于自己的优先级。从而最后使得高优先级的线程可以被CPU调用，执行。

### 任务

实现优先级捐赠这一操作。优先级捐赠有可能会是多个线程捐给一个线程，也有可能是出现嵌套捐赠。为了防止无限嵌套，也可以自己设置最多嵌套层数。上面说的“捐赠”只针对于`locks`。需要修改的两个函数是`thread_set_priority()` 和 `thread_get_priority()` 。

**任务总结：** 将FCFS变为抢占式优先级调度。

### 模块

#### 1.`list_insert_ordered()`

在阅读代码的时候，我们发现原本代码中将线程放入就绪队列的函数是`list_push_back()` ，这么做的问题是直接把线程放入队尾了，没有体现出线程的优先级。我们还发现有一个函数可以在将线程放入队列时体现函数的优先级，那就是`list_insert_ordered()` 这个函数。于是我们将原本在`thread.c` 中的`list_push_back` 都替换成 `list_insert_ordered()` 。

```c
/* Inserts ELEM in the proper position in LIST, which must be
   sorted according to LESS given auxiliary data AUX.
   Runs in O(n) average case in the number of elements in LIST. */
Void list_insert_ordered (struct list *list, struct list_elem *elem,
                     list_less_func *less, void *aux)
{
  struct list_elem *e;
 
  ASSERT (list != NULL);
  ASSERT (elem != NULL);
  ASSERT (less != NULL);
 
  for (e = list_begin (list); e != list_end (list); e = list_next (e))
    if (less (elem, e, aux))
      break;
  return list_insert (e, elem);
}


```

注意到这个函数在给线程进行入队操作的时候需要一个`list_less_fuc()* less` 这个参数，于是需要根据两个线程的优先级进行比较大小，然后选择应该插入到队列的位置。

由于我们想让每个线程都能在进入就绪队列的时候就已经是按照优先级顺序整理好的，而且在这个任务中所有的调度都是抢占式的，所以为了保证就绪队列的有序性，我们想要达到一个当一个线程优先级发生变化的时候，就重新调整其在就绪队列中的位置的效果。在阅读完代码后，我们发现有两个地方可以进行改变线程的优先级，一个是`thread_set_priority()` 函数，另一个地方是`thread_create()` 。所以我们在这两个函数中都加入`thread_yield()` 这个函数。这样就能在优先级发生变化的时候直接重新排列就绪队列中的线程，实现抢断式优先级调度了。

#### 2.优先级反转问题

优先级反转是指一个低优先级的任务持有一个被高优先级任务所需要的共享资源。高优先任务由于因资源缺乏而处于受阻状态，一直等到低优先级任务释放资源为止。而低优先级获得的CPU时间少，如果此时有优先级处于两者之间的任务，并且不需要那个共享资源，则该中优先级的任务反而超过这两个任务而获得CPU时间。如果高优先级等待资源时不是阻塞等待，而是忙循环，则可能永远无法获得资源，因为此时低优先级进程无法与高优先级进程争夺CPU时间，从而无法执行，进而无法释放资源，造成的后果就是高优先级任务无法获得资源而继续推进。

这个问题的解决方案就是优先级捐赠。先将高优先级赋给低优先级，在低优先级释放后，原本高优先级线程就可以继续执行了。

#### 3. 关于`lock` 

`lock` 类似于信号量，但是与信号量不同的地方在于每个`lock` 有归属，意味着一个`lock` 的值只能由拥有它的线程改变，并且值最大为1.

#### 4.总结需要改变优先级的情况

##### 1.单独捐赠

###### 情况

当前低优先级的线程占据了一个高优先级线程的锁。

###### 处理办法

直接将高优先级线程属性中的`priority` 赋值给低优先级线程即可。

##### 2.多重捐赠

###### 情况

多个高优先级的线程等待共同等待。

###### 处理办法

记录最高级优先级的线程，赋值给低优先级线程

##### 3.嵌套捐赠

###### 处理办法

每一层都看一下是否与当前线程相关，以此嵌套即可。

### 总结

如果当前占有锁的线程的优先级比新来到想要`acquire_lock()` 的线程低，就提高当前线程的优先级。如果这个锁还被其他的线程占据，将会递归的捐赠优先级，然后在释放后恢复原本的优先级。（因此要给线程结构体里加一个记录原本优先级的属性）。如果一个线程被许多优先级捐赠，就将自身优先级给成这些线程优先级中最大的那个。总而言之就是priority跟随lock变而变。

### 方案

1.给线程结构体加上一个是否捐赠的标记`bedonated` ，这个`bool` 变量记录着当前这个线程是否被捐赠过。在一个线程`acquire_lock()`时查看当前这个锁是否有`hloder` ，如果有，就比较优先级。如果优先级比当前线程低，就捐赠当前线程的优先级。如果这个锁还被别的锁锁着，就递归着捐赠优先级。在这个线程被释放掉的时候恢复未被捐赠时的优先级。如果一个线程被多个线程捐赠，就取这些线程中的优先级最大值为自己当前的优先级。


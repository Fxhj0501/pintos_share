#                  Pintos 笔记

 

## Part one. Alarm Clock

### 问题

出现忙等待（“busy wait"),出现空转现象。这回使得这个进程一直空转来检查当前时间并调用`thread_yield()`函数直到时间片结束。

### 目标

让它避免忙等待

## 模块

### 1. timer_sleep

#### 先调用`timer_ticks()` 

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

## 解决方案

换一种让线程等待的方式。`ticks` 表示要等待多久，所以当`ticks` 减为0的时候就代表线程结束等待。所以给当前线程加一个记录要等待多久的一个属性，然后当`ticks` 不为0的时候`block()` 线程，为0时进行`unblock()` 操作即可。










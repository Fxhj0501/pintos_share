#                  Pintos 笔记

 

## Part One. Alarm Clock

### 问题描述

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



## Part Two. Priority Scheduling

### 优先级定义

在Pintos中实现优先级调度。当一个线程被添加到比当前运行的线程优先级更高的就绪列表中时，当前线程应该立即将处理器分配给新线程。类似地，当线程在等待锁、信号量或条件变量时，应该首先唤醒优先级最高的等待线程。一个线程可以在任何时候提高或降低它自己的优先级，但是降低它的优先级以使它不再具有最高优先级，必须使它立即让出CPU。线程优先级从`PRI_MIN(0)`到`PRI_MAX(63)`。线程优先级是创建线程是传给`thread_create()`的参数，默认值为`PRI_DEFAULT(31)`。

### 问题描述

高级别的线程有时候因为一些原因永远不能得到CPU的使用权。这个问题的解决方法之一就是让高优先级的线程将自己的级别“捐赠”给低优先级别的线程，然后让原本低优先级的线程运行，在占据CPU的低优先级线程完成后被释放了，就再让高优先级的线程拿回属于自己的优先级。从而最后使得高优先级的线程可以被CPU调用，执行。

### 任务

实现优先级捐赠这一操作。优先级捐赠有可能会是多个线程捐给一个线程，也有可能是出现嵌套捐赠。为了防止无限嵌套，也可以自己设置最多嵌套层数。上面说的“捐赠”只针对于`locks`。需要修改的两个函数是`thread_set_priority()` 和 `thread_get_priority()` 。

### 解决方案

#### 1.将FCFS队列替换为优先级队列

先将`ready_list` 和`waiting_list` 的排序方式换成优先级的方式，即将`list_push_back` 换成`list_insert_ordered` ，为了配合`list_insert_ordered` 的使用，要自己创建比较两个线程之间优先级的函数，该函数是`bool` 型的。且每次优先级发生改变了以后，都要把线程重新插入到队列中去，即调用`thread_yield` 函数。注意`thread` 会出现在两个队列中，分别是`waiters` 以及`ready_list` 中。所以确保这两个队列都是按照优先级排序的并且调出线程时是按照优先级调出即可。

#### 2.优先级捐赠

由于考虑到线程有可能被捐赠优先级，所以给线程添加一个`inital_priority` 属性，以保留线程原始的优先级。一个线程有可能会拥有很多个锁，因此需要一个队列来存储这些锁以便释放时可以找到这些锁，因此加入一个`hold_locks` 这个队列。因此为了能将锁放入这个队列，在`lock` 结构体中添加类型为`list_elem` 的属性`elem` 。最后还要添加一个指向被当前线程捐赠线程的属性`donate_thread` ，这个属性是用来在嵌套捐赠优先级的时候，能够将锁的拥有者以及被锁的拥有者捐赠过的线程这条链上的所有线程的优先级都提升到当前线程的优先级以便当前线程能取得锁。(上面这段话感觉不能很恰当描述。举个如同官方档案里面说的，H等M，M等L，假如当前线程是H，那么就可以根据`donate_thread` 将自己的优先级捐赠给M，再通过M的`donate_thread` 将H自己的优先级捐赠给L。)因此`lock_acquire()` 中，就可以通过判断是否要捐赠优先级来通过`donate_thread` 来嵌套捐赠优先级同时获得锁。在释放锁的时候，如果当前线程被其他等待当前线程释放锁的线程捐赠，那么就找这些等待者中最大优先级赋值给当前线程。如果没有等待的线程或者这些线程的优先级都没有当前线程的`inital_priority` 大，那么就使`priority = intial_priority` 即可。并且在`lock_release` 中还要将`lock_holder` 设为`NULL` ，`sema_up(&lock->semaphore)` ，这样才算真正的释放锁。并且也要把当前线程的`donate_thread` 设为`NULL` 。最后还要修改`thread_set_priority()` 。因为现在添加了`inital_priority` 这个属性，所以原本将`new_priority` 赋值给`priority` 的操作应该赋值给`inital_priority` 。如果`new_priority` 大于现在的`priority` 或者当前线程没有被捐赠过,那么就将此时的`priority` 也给变成`new_priority` 。



## Part Three. Avanced Scheduler

### 问题描述

因为每个线程各自功能的不同，所以对于CPU的需求也不同。有的是I/O密集型，有的是需要长时间占用CPU的，而有的是这两者的综合情况。所以我们需要设计一种调度器来调度它们。

### 任务

上述说的调度器有很多就绪队列，这些队列是为了让不同优先级的线程来排队的。在调度时，要先从优先级最高的非空就绪队列中调出线程。最高优先级队列中的线程是按照“循环制”排队的。

#### 介绍1.Nice值

一个线程有一个类型为`int` 的nice值，正nice值最大为20，负nice值最小为-20。正nice值会使当前线程的优先级降低来让出CPU给其他线程用，负nice值会抢走别的线程使用CPU的时间。nice值为0时，不会对线程有影响。(

没作用)。每个线程的初始nice值都是0。pinots提供了两个函数框架可以使用：`thread_get_nice()` 和 `thread_set_nice()` 。

####  介绍2.计算优先级

因为线程的优先级是0～63，所以也有64个就绪队列。在计算线程优先级时，系统会自动的对于所有线程每四个`clock tick` 一计算。计算的公式是：

*Priority = PRI_MAX - (recent_cpu/4) - (nice2)* 

其中的*recent_cpu* 下面会介绍。这个公式可以防止线程饥饿。

#### 介绍3.计算*recent_cpu*

计算*recent_cpu* 的方式：

x(0) =f(0)

x(t) =ax(t−1) + (1−a)f(t)

a=k/(k+ 1)

线程初始的`recent_cpu` 是0或者父亲线程的值。每次中断一发生，`recent_cpu` 值就加1，只针对于正在运行的线程。每秒都会用这个公式重新计算*recet_cpu* 的值，针对于每个线程。公式如下：

*recent_cpu= (2*load_avg)/(2*load_avg+ 1) recent_cpu+nice*



#### 总结公式

上面的似乎不是很重要，在官方文档里，*“B.5 Summary"* 对于公式进行了总结，总共有三个公式：

1.*priority= PRI_MAX - (recent_cpu/ 4) - (nice\* 2)*

2.*recent_cpu= (2*load_avg)/(2\*load_avg+ 1)* *recent_cpu+nice*

3.*load_avg= (59/60)\*load_avg+ (1/60)\*ready_threads*

其中公式1是每四个ticks计算一次，获得当前线程的`nice` 值的方式是调用`thread_get_nice()` 。这个公式需要对每个线程都计算。（所以可能要调用`thread_foreach()` 函数？）

对于公式2，每个`tick` 都要计算一次`recent_cpu` ，每个线程的`recent_cpu` 都+1,每秒都要应用公式2求一下所有线程的`recent_cpu` 。

对于公式3，每秒要调用一次。这个量不属于任何一个线程，属于对*system* 的一种描述。



#### 解决方案

pintos官方档案里说了`priority` ,`nice` 以及`ready_threads` 都是整数型，`recent_cpu` 和`load_avg` 都是实数，也就是对应在计算机里的浮点数。因为`nice` 和`recent_cpu` 都是线程的属性，所以要加在线程的结构体中。`load_avg` 是描述系统的量，所以应加在头文件中。

实现对于定点数的计算功能。由于不确定传进来的数的类型，也不确定返回数的类型，C语言中也没有`fixed_point` 这个类型，并且C语言中也没有函数重载，所以只能用宏定义函数来实现。实现内容按照官方档案上的执行就行，注意`<<` 的运算优先顺序，所以需要加上括号。










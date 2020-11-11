#                  Pintos 笔记



## Part one. Alarm Clock

### 问题

出现忙等待（“busy wait"),出现空转现象。这回使得这个进程一直空转来检查当前时间并调用`thread_yield()`函数直到时间片结束。

### 目标

让它避免忙等待

### 参考

[Pintos-斯坦福大学操作系统Project详解](https://blog.csdn.net/denghuang8508/article/details/101357600?ops_request_misc=%257B%2522request%255Fid%2522%253A%2522160502078219195264764271%2522%252C%2522scm%2522%253A%252220140713.130102334..%2522%257D&request_id=160502078219195264764271&biz_id=0&utm_medium=distribute.pc_search_result.none-task-blog-2~all~top_click~default-1-101357600.first_rank_ecpm_v3_pc_rank_v2&utm_term=pintos&spm=1018.2118.3001.4449)

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

第三行跳出while的条件：在x个时间片中一直进行`thread_yield` 直到进行了x个时间片。

#####  


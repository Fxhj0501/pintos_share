#             Pintos 各个函数的功能

1. `intr_get_level()` 表示当前中断状态
2. `intr_disable()` 保证当前操作是原子的
3. `list_push_back` 将当前线程放入就绪队列
4. `intr_context()`判断当前是否是外部中断
5. `running thread()` 返回当前线程的开头指针
6. `thread_yield()` 将线程重新扔进就绪队列中


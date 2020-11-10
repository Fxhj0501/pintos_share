#                  Pintos 笔记



## Part one. Alarm Clock

### 问题

出现忙等待（“busy wait"),出现空转现象。这回使得这个进程一直空转来检查当前时间并调用`thread_yield()`函数直到时间片结束。

### 目标

让它避免忙等待

## 模块

### 1. timer_sleep








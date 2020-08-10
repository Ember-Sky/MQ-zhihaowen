@echo off
set filepath= D:\Desktop\MQtest\test01-MQ\priority_test\publish_priority\Debug\publish_priority.exe
set mode=1
set publish_num=3
set msg_time=2000
set msg_len=10

for /L %%i in (1,1,%publish_num%) do (
	start %filepath% %mode% %%i %msg_time% %%i %msg_len%
)

::pause

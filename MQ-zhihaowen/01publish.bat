@echo off
set filepath=D:\Desktop\MQtest\test01-MQ\publish01\Debug\publish01.exe
set mode=1
set publish_num=2
set msg_time=1
set msg_len=100

for /L %%i in (1,1,%publish_num%) do (
	start %filepath% %mode% %%i %msg_time% %msg_len%
)

::pause

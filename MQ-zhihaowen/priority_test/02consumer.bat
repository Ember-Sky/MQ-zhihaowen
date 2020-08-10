@echo off
set filepath=D:\Desktop\MQtest\test01-MQ\priority_test\consumer_priority\Debug\consumer_priority.exe
set mode=1
set consumer_num=1

for /L %%i in (1,1,%consumer_num%) do (
	start %filepath% %mode% %%i
)
::pause
::@cmd.exe
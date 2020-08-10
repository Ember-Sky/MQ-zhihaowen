@echo off
set filepath=D:\Desktop\MQtest\test01-MQ\consumer01\Debug\consumer01.exe
set mode=1
set consumer_num=2

for /L %%i in (1,1,%consumer_num%) do (
	start %filepath% %mode% %%i
)
::pause
::@cmd.exe
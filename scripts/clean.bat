c:
cd \windows\temp\
for %%i in (*) do if not %%i == script.bat del /s /q %%i
for /D %%i in (*) do del /s /q %%i

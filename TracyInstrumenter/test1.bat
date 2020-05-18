@echo off
set /p input=how much on 10?
if %input% LSS 4 (
msg * Less Than 4
) else (
 if %input% LSS 8 msg * Less Than 8 )
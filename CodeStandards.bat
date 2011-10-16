@ECHO OFF
REM CodeStandards.bat
REM runs some elementary cross-checks on Franci's source code

echo Disallow DeleteNSlotsAt(1, as bulky form of DeleteIdx
grep -F "DeleteNSlotsAt(1," *.hxx *.cxx
if not errorlevel 1  echo Forbidden code construct DeleteNSlotsAt(1,...) found.  Please replace with DeleteIdx(...).


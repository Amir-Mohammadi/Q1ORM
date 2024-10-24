@echo off
cls

@REM Checking the number of input arguments.
set arg_count=0
for %%x in (%*) do Set /A arg_count+=1

if NOT "%arg_count%" == "4" (
    echo "0|The number of input arguments should be 4 including (file path or file download location, extract path, username, password)"
    pause
    exit /b 0
)

set download_to=%1
set extract_to=%2
set username=%3
set password=%4

if NOT EXIST "%download_to%" (
    echo "1|The file path or file download path is not correct."
    pause
    exit /b 0
)

if NOT EXIST "%extract_to%" (
    echo "2|The path of the desired location to extract the file is not correct."
    pause
    exit /b 0
)

@REM Checks if the last character is equal to "/" or "\" and deletes that character. //// for download_to
set download_to_last_character=%download_to:~-1%

if /I "%download_to_last_character%" == "/" (
    set download_to=%download_to:~0,-1%
) else if /I "%download_to_last_character%" == "\" (
    set download_to=%download_to:~0,-1%
)

@REM Checks if the last character is equal to "/" or "\" and deletes that character. //// for extract_to
set extract_to_last_character=%extract_to:~-1%

if /I "%extract_to_last_character%" == "/" (
    set extract_to=%extract_to:~0,-1%
) else if /I "%extract_to_last_character%" == "\" (
    set extract_to=%extract_to:~0,-1%
)

set "zip_file=%download_to%\postgresql.zip"
set "cluster_db=%extract_to%\pgsql\bin\clusterdb.exe"
set "psql_folder=%extract_to%\pgsql\bin"
set "data_folder=%extract_to%\pgsql\data"

echo "3|Checking postgresql folder is exists ..."

if EXIST "%cluster_db%" (
  echo "4|PostgreSQL is already installed at "%extract_to%/""
  pause
  exit /b 0
)

echo "5|Checking that the zip file exists (postgresql.zip) ..."

cd /D "%download_to%/"

if EXIST "%zip_file%" (
    echo "6|postgresql.zip file is exist."
) else (
    echo "7|Start downloading postgresql.zip ..."

    powershell -Command "Invoke-WebRequest https://sbp.enterprisedb.com/getfile.jsp?fileid=1258697 -Outfile postgresql.zip"

    if ERRORLEVEL 1 (
        echo "8|Download Fail, Please Check Your Internet Connection!"

        DEL "%zip_file%"
        pause

        exit /b 1
    )
)

if NOT EXIST "%zip_file%" (
    echo "9|The Zip File "%zip_file%" does not exist."
    pause
    exit /b 0
)

if not exist "%extract_to%/" (
    mkdir "%extract_to%/"
)

echo "10|Extracting ..."

if EXIST "%cluster_db%" (
    echo "11|Clusterdb has exist."
) else (
    powershell -nologo -noprofile -command "& { Add-Type -A 'System.IO.Compression.FileSystem'; [IO.Compression.ZipFile]::ExtractToDirectory('%zip_file%', '%extract_to%/'); }"
)

del "%zip_file%"

echo "12|Extraction Complete."

cd /D "%psql_folder%"

echo %password%>> "pass.conf"

call initdb.exe -U %username% -A password --pwfile=pass.conf -E utf8 -D ..\data
call pg_ctl -D ^"^.^.^\data^" -l logfile start
call pg_ctl.exe register -N PostgreSQL -D %data_folder%

del "pass.conf"

echo "13|All Process Done Successfuly!"

pause
exit /b 0

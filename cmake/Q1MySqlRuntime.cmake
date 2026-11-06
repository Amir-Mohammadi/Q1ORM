include_guard(GLOBAL)

function(q1orm_deploy_mysql_runtime target)
    if(NOT WIN32)
        return()
    endif()

    set(Q1ORM_MYSQL_RUNTIME_DIR "" CACHE PATH
        "Directory containing the MySQL client DLL used by Qt's QMYSQL plugin")
    set(runtime_hints "${Q1ORM_MYSQL_RUNTIME_DIR}" "$ENV{Q1ORM_MYSQL_RUNTIME_DIR}")
    foreach(configuration IN ITEMS RELEASE DEBUG)
        get_target_property(qt_core_dll Qt6::Core "IMPORTED_LOCATION_${configuration}")
        if(qt_core_dll)
            get_filename_component(qt_bin_dir "${qt_core_dll}" DIRECTORY)
            list(APPEND runtime_hints "${qt_bin_dir}")
        endif()
    endforeach()
    file(GLOB mysql_installations LIST_DIRECTORIES TRUE
        "$ENV{ProgramW6432}/MySQL/MySQL Server *"
        "$ENV{ProgramFiles}/MySQL/MySQL Server *")
    list(APPEND runtime_hints ${mysql_installations})
    list(REMOVE_DUPLICATES runtime_hints)

    # Keep discovery local so changing Q1ORM_MYSQL_RUNTIME_DIR takes effect.
    unset(q1orm_mysql_client CACHE)
    find_file(q1orm_mysql_client NAMES libmysql.dll
        HINTS ${runtime_hints} PATH_SUFFIXES lib bin)
    set(mysql_client "${q1orm_mysql_client}")
    unset(q1orm_mysql_client CACHE)
    if(NOT mysql_client)
        message(STATUS
            "${target}: MySQL client runtime not found; set Q1ORM_MYSQL_RUNTIME_DIR to enable QMYSQL")
        return()
    endif()

    get_filename_component(client_dir "${mysql_client}" DIRECTORY)
    get_filename_component(client_parent "${client_dir}" DIRECTORY)
    set(runtime_dlls "${mysql_client}")
    set(runtime_names libmysql.dll)
    # MySQL installations commonly put OpenSSL beside mysql.exe, while the
    # client DLL lives in lib/. These are native dependencies of libmysql.dll.
    foreach(dependency_dir IN ITEMS "${client_dir}" "${client_parent}/bin" "${client_parent}/lib")
        file(GLOB dependency_dlls
            "${dependency_dir}/libssl*.dll" "${dependency_dir}/libcrypto*.dll"
            "${dependency_dir}/zlib*.dll" "${dependency_dir}/zstd*.dll"
            "${dependency_dir}/libzstd*.dll")
        foreach(dependency IN LISTS dependency_dlls)
            get_filename_component(dependency_name "${dependency}" NAME)
            string(TOLOWER "${dependency_name}" dependency_name)
            if(NOT dependency_name IN_LIST runtime_names)
                list(APPEND runtime_dlls "${dependency}")
                list(APPEND runtime_names "${dependency_name}")
            endif()
        endforeach()
    endforeach()

    add_custom_command(TARGET ${target} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${runtime_dlls} "$<TARGET_FILE_DIR:${target}>"
        COMMENT "Copying MySQL client runtime for ${target}"
        VERBATIM)
    message(STATUS "${target}: MySQL client runtime: ${mysql_client}")
endfunction()

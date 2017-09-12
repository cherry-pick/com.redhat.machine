io_systemd_sysinfo_sources = files('''
        main.c
        util.h
'''.split())

io_systemd_sysinfo_varlink_c_inc = custom_target(
        'io.systemd.sysinfo.varlink',
        input : 'io.systemd.sysinfo.varlink',
        output : 'io.systemd.sysinfo.varlink.c.inc',
        command : [varlink_wrapper_py, '@INPUT@', '@OUTPUT@'])

exe = executable(
        'io.systemd.sysinfo',
        io_systemd_sysinfo_sources,
        io_systemd_sysinfo_varlink_c_inc,
        dependencies : [
                libvarlink
        ],
        install : true)

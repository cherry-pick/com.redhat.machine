com_redhat_machine_sources = files('''
        main.c
        util.h
'''.split())

com_redhat_machine_varlink_c_inc = custom_target(
        'com.redhat.machine.varlink',
        input : 'com.redhat.machine.varlink',
        output : 'com.redhat.machine.varlink.c.inc',
        command : [varlink_wrapper_py, '@INPUT@', '@OUTPUT@'])

exe = executable(
        'com.redhat.machine',
        com_redhat_machine_sources,
        com_redhat_machine_varlink_c_inc,
        dependencies : [
                libvarlink
        ],
        install : true)

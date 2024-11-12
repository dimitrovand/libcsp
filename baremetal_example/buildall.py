import subprocess

if __name__ == '__main__':
    targets = ['baremetal_example/server_client', 'baremetal_example/server_ping', 'baremetal_example/server_rdp']
    builddir = 'build'

    cmake_setup = ['cmake', '-GNinja', '-DCMAKE_SYSTEM_NAME=Generic', '-B' + builddir]
    cmake_compile = ['ninja', '-C', builddir]
    subprocess.check_call(cmake_setup)
    subprocess.check_call(cmake_compile + targets)

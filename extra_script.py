import os
Import("env", "projenv")
from pathlib import Path

class bcolors:
    HEADER = '\033[95m'
    OKBLUE = '\033[94m'
    OKCYAN = '\033[96m'
    OKGREEN = '\033[92m'
    WARNING = '\033[93m'
    FAIL = '\033[91m'
    ENDC = '\033[0m'
    BOLD = '\033[1m'
    UNDERLINE = '\033[4m'
    UP = '\033[1A'

def post_program_action(source, target, env): 
    
    if os.path.exists("binary")==False:os.mkdir("binary", 0o666)
    objdump="@copy "
    src_elf=env.subst("\"${BUILD_DIR}\\${PROGNAME}.exe\"")
    new_name=os.path.basename(os.path.normpath(env.subst('${PROJECT_DIR}'))).strip()
    cmd=objdump+src_elf+" \"${PROJECT_DIR}\\binary\\"+new_name+".exe\" > nul"  
    env.Execute(cmd)
    objdump="@del "
    src_elf=env.subst("\"${BUILD_DIR}\\${PROGNAME}.exe\"")
    cmd=" ".join([objdump,src_elf])  
    env.Execute(cmd)
    os.chdir("binary")
    print(bcolors.HEADER+"Starting application:\n")
    env.Execute("@"+new_name+".exe")
    print("\n"+bcolors.OKBLUE+"Application ended.\n\n")


env.AddPostAction("$BUILD_DIR/program.exe", post_program_action)
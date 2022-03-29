import os
os.system("cd ../../hello-world")
os.system("ARCH_PATH=../../../arch make TARGET=iotlab BOARD=m3 savetarget")
os.system("ARCH_PATH=../../../arch make")

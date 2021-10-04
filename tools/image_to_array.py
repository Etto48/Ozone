#!/usr/bin/python3

from PIL import Image
import numpy as np
import os 
from natsort import natsorted

folder_name=input("Folder Name:")

with open(folder_name+"/out.txt","w") as f:
    f.write("{\n")
    files = natsorted(os.listdir(folder_name))
    for file_name in files:
        if(file_name[-4:]==".png"):                
            I=np.asarray(Image.open(folder_name+"/"+file_name))

            f.write("{\n")
            for row in I:
                for pixel in row:
                    if(len(pixel)==3):
                        f.write(f"0xff{pixel[0]:02X}{pixel[1]:02X}{pixel[2]:02X}")
                    else:
                        f.write(f"0x{pixel[3]:02X}{pixel[0]:02X}{pixel[1]:02X}{pixel[2]:02X}")
                    f.write(",")
                f.write("\n")
            f.write("},\n")
    f.write("};")
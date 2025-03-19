#!python3
# coding=utf-8

import os
import sys
import struct
# import lzma
# import bsdiff4_2ya
from binascii import crc32
from os.path import getsize
from optparse import OptionParser
# from base import logFactory

# logging = logFactory.get()

def human_bytes(n):
    """
    return the number of bytes 'n' in more human readable form
    """
    if n < 1024:
        return '%i B' % n
    k = (n - 1) / 1024 + 1
    if k < 1024:
        return '%i KB' % k
    return '%.2f MB' % (float(n) / (2 ** 20))


def insert_head(src_data, flash_size, buf_size, part_num, block_size):
 
    patch_header = struct.pack(">9I28x",#>3I4B2I
                               0x74757961,
                               0xFFFFFFFF,
                               flash_size,
                               1,
                               buf_size,
                               part_num,
                               block_size,
                               len(src_data),
                               crc32(src_data))
    
    print("src_data size:{}, crc32 src_data:{}, flash_size:{}"
            .format(len(src_data),  crc32(src_data), flash_size))
    
    return (src_data + patch_header)


def main_format_bin():
    p = OptionParser(
        usage="usage: %prog [options] SRC DST PATCH",
        description=("generate a BSDIFF4-format PATCH from SRC to DST "
                     "and write it to PATCH"))

    p.add_option('-v', "--verbose",
                 action="store_true")

    opts, args = p.parse_args()
    src_path = args[0]
    dst_path = args[1]
    flash_size = args[2]
    buf_size = args[3]
    part_num = args[4]
    block_size = args[5]
    flash_size = int(flash_size, 16)
    buf_size = int(buf_size, 16)
    part_num = int(part_num)
    block_size = int(block_size, 16)
    if (buf_size % block_size != 0):
        # logging.err("buf_size:{} err... must {} align", buf_size, block_size)
        return -1
        
    #return -1
    print("src_path:{}, dst_path:{}, flash_size:{}"
            .format(src_path, dst_path, flash_size))
            
    with open(src_path, 'rb') as f:
        src_data = f.read()
   
    patch_io = open(dst_path, "wb")
    patch_io.write(insert_head(src_data, flash_size, buf_size, part_num, block_size))
    patch_io.close()

    if len(args) != 6:
        p.error('requies 6 arguments, try -h')

if __name__ == "__main__":
    main_format_bin()

#!/bin/bash

# 遍历当前文件夹下的所有文件
for file in *; do
    # 判断是否为文件
    if [ -f "$file" ]; then
        # 使用file命令打印文件信息
        echo "File: $file\n"
        file "$file"
        echo
    fi
done

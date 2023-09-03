#!/bin/bash

# 设置最大尝试次数
max_attempts=100

# 初始化尝试计数器
attempts=0

while [ $attempts -lt $max_attempts ]; do
  # 增加尝试计数器
  ((attempts++))

  echo "尝试 $attempts/$max_attempts 拉取仓库..."

  # 使用 git 拉取仓库
  git fetch

  # 检查拉取是否成功
  if [ $? -eq 0 ]; then
    echo "成功拉取仓库。"
    exit 0  # 成功，退出脚本
  else
    echo "拉取失败。"
    echo "等待一段时间后重试..."
    sleep 10  # 等待一段时间后重试（可以根据需要调整等待时间）
  fi
done

# 如果达到最大尝试次数仍然失败，输出错误消息并退出
echo "达到最大尝试次数，无法成功拉取仓库。"
exit 1
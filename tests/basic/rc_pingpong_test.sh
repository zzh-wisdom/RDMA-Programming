# size_less_1k=(64 256)

# size=${size_less_1k[0]}
# for((i=0;i<2;i++)); do
#     size=${size_less_1k[${i}]}
#     echo "size: ${size}"
#     ibv_rc_pingpong --ib-port=1 --size=${size} --iters=1000000 &
#     ibv_rc_pingpong --ib-port=2 --size=${size} --iters=1000000 localhost
#     wait
# done

# wait

# for((i=1;i<=1024;i*=4)); do
#     size=$((i*1024))
#     echo "size: ${size}"
#     ibv_rc_pingpong --ib-port=1 --size=${size} --iters=1000000 &
#     ibv_rc_pingpong --ib-port=2 --size=${size} --iters=1000000 localhost
#     wait
# done

# mtu=${1}
# size=$((${2}))
# echo "mtu: ${mtu}, size: ${size}"
# ibv_rc_pingpong --ib-port=1 --iters=1000000 --mtu=${mtu} --size=${size} &
# ibv_rc_pingpong --ib-port=2 --iters=1000000 --mtu=${mtu} --size=${size} localhost
# wait

# rx_depth=${1}
# size=$((${2}))
# echo "rx_depth: ${rx_depth}, size: ${size}"
# ibv_rc_pingpong --ib-port=1 --iters=1000000 --rx-depth=${rx_depth} --size=${size} &
# ibv_rc_pingpong --ib-port=2 --iters=1000000 --rx-depth=${rx_depth} --size=${size} localhost
# wait

# 使用事件

# size=$((${1}))
# echo "--events, size: ${size}"
# ibv_rc_pingpong --ib-port=1 --iters=1000000 -e --size=${size} &
# ibv_rc_pingpong --ib-port=2 --iters=1000000 -e --size=${size} localhost
# wait

# echo "--no events, size: ${size}"
# ibv_rc_pingpong --ib-port=1 --iters=1000000 --size=${size} &
# ibv_rc_pingpong --ib-port=2 --iters=1000000 --size=${size} localhost
# wait

# odp

# size=$((${1}))
# echo "--no odp, size: ${size}"
# ibv_rc_pingpong --ib-port=1 --iters=1000000 --size=${size} &
# ibv_rc_pingpong --ib-port=2 --iters=1000000 --size=${size} localhost
# wait

# size=$((${1}))
# echo "--odp, size: ${size}"
# ibv_rc_pingpong --ib-port=1 --iters=1000000 --odp --size=${size} &
# ibv_rc_pingpong --ib-port=2 --iters=1000000 --odp --size=${size} localhost
# wait

# size=$((${1}))
# echo "--iodp, size: ${size}"
# ibv_rc_pingpong --ib-port=1 --iters=1000000 --iodp --size=${size} &
# ibv_rc_pingpong --ib-port=2 --iters=1000000 --iodp --size=${size} localhost
# wait


# dm 不支持

# size=$((${1}))
# echo "--no odp, size: ${size}"
# ibv_rc_pingpong --ib-port=1 --iters=1000000 --dm --size=${size} &
# ibv_rc_pingpong --ib-port=2 --iters=1000000 --dm --size=${size} localhost
# wait


# ts

# size=$((${1}))
# echo "--no ts, size: ${size}"
# ibv_rc_pingpong --ib-port=1 --iters=1000000 --size=${size} &
# ibv_rc_pingpong --ib-port=2 --iters=1000000 --size=${size} localhost
# wait

# size=$((${1}))
# echo "--ts, size: ${size}"
# ibv_rc_pingpong --ib-port=1 --iters=1000000 --ts --size=${size} &
# ibv_rc_pingpong --ib-port=2 --iters=1000000 --ts --size=${size} localhost
# wait


# new_send 实验室的网卡使用不了

size=$((${1}))
echo "--no new_send, size: ${size}"
ibv_rc_pingpong --ib-port=1 --iters=1000000 --size=${size} &
ibv_rc_pingpong --ib-port=2 --iters=1000000 --size=${size} localhost
wait

size=$((${1}))
echo "--new_send, size: ${size}"
ibv_rc_pingpong --ib-port=1 --iters=1000000 --new_send --size=${size} &
ibv_rc_pingpong --ib-port=2 --iters=1000000 --new_send --size=${size} localhost
wait

# Read which interface to clean
crop="0"
ocr="0"
while test $# -gt 0; do
    case "$1" in 
        -i )
            interface=$2
            shift 2
        ;;
        -m )
            mode=$2
            shift 2
        ;;
        -v )
            video=$2
            shift 2
        ;;
        -crop )
            crop="1"
            shift 1
        ;;
        -ocr )
            ocr="1"
            shift 1
        ;;
        -lo )
            lower=$2
            shift 2
        ;;
        -up )
            upper=$2
            shift 2
        ;;
        -thres )
            threshold=$2
            shift 2
        ;;
        *)
            break
        ;;
    esac
done;

# if [ "$mode" == "clean" ];
# then
#     rm -r ./samples/$interface/images/*
#     rm -r ./samples/$interface/log/*
# fi

make
if [ "$mode" == "access" ];
then
    ./vizaccess $interface $video $threshold
fi

if [ "$mode" == "build" ];
then
    ./vizbuild $interface $video $lower $upper $crop $ocr
fi

# ./viz $interface $video $mode $lower $upper

# while test $# -gt 0; do
#     case "$1" in 
#         -i )
#             interface=$2
#             shift 2
#         ;;
#         -clean )
#             mode="clean"
#             shift 1
#         ;;
#         *)
#             break
#         ;;
#     esac
# done;

# if [ "$mode" == "clean" ];
# then
#     rm -r ./samples/$interface/images/*
#     rm -r ./samples/$interface/log/*
# fi

# ./viz

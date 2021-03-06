#!/bin/sh

#custom_edit_code 

docker_name=golang
dparams="$dparams --privileged=true "
image_name=qdocker/golang:7
docker_name=golang
screen_name=docker${docker_name}
blackhole_real_dir="../code/$screen_name"
o_ports=" -p 9001:9001"

#end custom_edit_code 



SYSTEM=$(uname -s)
if [ $SYSTEM == "Darwin" ] 
then
	Greadlink=greadlink
else
	Greadlink=readlink
fi
vdir=$($Greadlink -f $blackhole_real_dir)	
echo $vdir
mkdir -p $vdir
dparams="--name $docker_name -i -t $o_ports  -v $vdir:/blackhole -w /blackhole"

r=$(docker exec $docker_name /bin/sh -c "echo OK" 2>&1 )
if [ "$r" == "OK" ]
then
	echo "already running  "
	screen -x $screen_name
elif [[ "$r" =~ "no such id" ]]
then
   echo "not exist , not running , run  "
	 screen -S $screen_name docker run $dparams $image_name
else
    echo "exist , not running , run and attach "
    screen -S $screen_name /bin/bash -c "docker start $docker_name && docker attach $docker_name"
fi

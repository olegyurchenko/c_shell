echo $0
#__DEBUG__=1
#__DEBUG_LEX__=1
echo -n "Test1 "
if [ 1 != 2 ] ;then echo "Ok";else echo "failed"; fi

echo -n "Test2 "
[[ 1 == 2 ]]
if [ ! $_ ] ; then echo "failed";else echo "Ok"; fi


echo -n "Test3 "
if [ 1 == 1 -o 2 == 3 ] ;then echo "Ok";else echo "failed"; fi

echo -n "Test4 "
if [ 1 == 1 -a 2 == 3 ] ; then echo "failed";else echo "Ok"; fi

echo -n "Test5 "
if [ \( 1 == 1 -a 2 == 3 \) -o \( 3 != 3 \) ] ; then echo "failed";else echo "Ok"; fi

echo -n "Test6 "
if [ ! \( 1 == 1 -a 2 == 3 \) -a \( 1 == 2 \) ] ; then echo "failed";else echo "Ok"; fi


echo -n "Test7 "
if [ ! ! \( \( 44 -lt 101 \) -a 'xxx' == "xxx" -a 'yyx' != 'yyz'  \) -a \( 0 -gt -101 \) ] ; then echo "Ok";else echo "failed"; fi

echo $0

x=$((1 + (2 * 3 & 1)))
echo -n "Test 1 "
if [ $x == 1 ] ; then echo Ok ; else echo Failed ; fi

x=$((y=x+1)) 
echo -n "Test 2 "
if [ $x == 2 -a $y == 2 ] ; then echo Ok ; else echo Failed ; fi


x=1
x=$((y=++x+1)) 
#echo $x $y
echo -n "Test 3 "
if [ $x == 3 -a $y == 3 ] ; then echo Ok ; else echo Failed ; fi


x=1
x=$((y=x++)) 
#echo $x $y
echo -n "Test 4 "
if [ $x == 1 -a $y == 1 ] ; then echo Ok ; else echo Failed ; fi


x=1
y=0
x=$((y=x&&y)) 
#echo $x $y
echo -n "Test 5 "
if [ $x == 0 -a $y == 0 ] ; then echo Ok ; else echo Failed ; fi


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

_=$((x=((0x33^0x11)+111-122==23)&&(1+2+3*3==12)&&(04553207==0x12d687)))
echo -n "Test 4 "
if [ $x == 1 ] ; then echo Ok ; else echo Failed ; fi

_=$((0x12345678^0x12345678|0x3456789&0x45678912<<3>>5==5324864))
echo -n "Test 5 "
if [ $x == 1 ] ; then echo Ok ; else echo Failed ; fi

n=10
i=""
while let n=n-1 ; do
let i=++i
#echo $n $i
done
echo -n "Test 6 "
#echo $i
if [ $i == 9 ] ; then echo Ok ; else echo Failed ; fi

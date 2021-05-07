x=""
x="" x=$(echo 1111)
 echo -n "Test1 "
if [ "$x" == "1111" ] ; then echo Ok ; else echo Failed ; fi

echo -n "Test2 "
if [ "$x" == `echo 1111` ] ; then echo Ok ; else echo Failed ; fi

#__DEBUG__=1
x="" 
x=$(echo $(echo $(echo $(echo $(echo $(echo 22222222222222))))))
echo -n "Test3 "
if [ "$x" == "22222222222222" ] ; then echo Ok ; else echo Failed ; fi

x=''
for i in `seq 5`;do
x=${x}$i
done

 echo -n "Test4 "
if [ "$x" == "12345" ] ; then echo Ok ; else echo Failed ; fi

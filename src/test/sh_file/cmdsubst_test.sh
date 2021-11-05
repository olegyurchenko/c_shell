echo $0

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
#__DEBUG_LEX__=1
x=${x}`echo $i`
#__DEBUG_LEX__=0
done


echo -n "Test4 "
if [ "$x" == "12345" ] ; then echo Ok ; else echo Failed ; fi


x=5555
x="123` echo 45`"
#echo "x=>$x<"
echo -n "Test5 "
if [ "$x" == "12345" ] ; then echo Ok ; else echo Failed ; fi


#__DEBUG_LEX__=1
echo -n "Test6 "
x="Quote test \"111\""
if [ "$x" == "Quote test \"111\"" ] ; then echo Ok ; else echo Failed ; fi

echo $0
__DEBUG__=1
x=0
while [ "$x" -ne 1111 ]; do
x="${x}1"
if [ "$x" == "11111" ] ; then break; fi
done
echo $x

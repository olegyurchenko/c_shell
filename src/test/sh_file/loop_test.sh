# while, for, break, continue tests
#__DEBUG__ = 1

while true ; do
  echo -n "While test1 "
  break
done
echo "Ok"

while true ; do
  echo -n "While test2 "
  if true ; then break ; fi
done
echo "Ok"

x=""
while true ; do
  if [ "$x" != 1 ] ; then
    echo -n "While test3 "
  else
    break
    echo "Test3 failed <1>"
  fi
  x=1
  continue
  echo "Test3 failed <2>"
done
echo "Ok"

#__DEBUG__=1
x=""
while true ; do
  if [ "$x" == 1 ] ; then
    break
    echo "Test4 failed <1>"
  else
    echo -n "While test4 "
    x=1
    continue
  fi
  echo "Test4 failed <2>"
done
echo "Ok"

x=""
until false ; do
  if [ "$x" == 1 ] ; then
    break
    echo "Test5 failed <1>"
  else
    echo -n "Until test5 "
    x=1
    continue
  fi
  echo "Test5 failed <2>"
done
echo "Ok"


x=""
for y in 1 2 3 4 5 ; do
  if [ "$y" == 1 ] ; then
    echo -n "For test6 "
    continue
  fi
  x="${x}${y}"
done

if [ "$x" == "2345" ] ; then
    echo "Ok"
else
    echo "Failed: $x"
fi

while true ; do
  x=""
  break
done

#__DEBUG__=1
#__DEBUG_CACHE__=1

echo -n "For test7 "
x=""
for y in 0 1 2 3 ; do
  for z in 0 1 2 3 ; do
    x="${x}${z}"
  done
  while false ; do
    x=""
    break
  done
  while true ; do
    if true ; then
        break
    fi
    x=""
  done
done

if [ "$x" == "0123012301230123" ] ; then
    echo "Ok"
else
    echo "Failed: $x"
fi


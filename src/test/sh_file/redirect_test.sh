echo $0
#__DEBUG__=1
echo -n 'Test1 '
echo 'Ok' |  tee | tee | tee

echo -n Test2> test
echo -n ' ' >> test
echo Ok >> test
tee < test



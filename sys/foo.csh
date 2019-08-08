foreach file (*.c)
echo $file >> LOG
diff $file /uvax/usr/src/sys/sys/$file >> LOG
end

for file in scripts/*.gp
do
	echo "plotting $file"
	gnuplot < $file
done

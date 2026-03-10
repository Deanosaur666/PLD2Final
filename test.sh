#! /bin/bash
books=(Hound Frankenstein Dracula)

for book in "${books[@]}"
do
	echo "$book" > "log_${book}.txt"
	echo "$book"
	for p in {1..30}
	do
		echo "P: $p" >> "log_${book}.txt"
		echo "P: $p" # so we can monitor progress
		for i in {1..20}
		do
			mpirun -n $p --oversubscribe titlefreq ${book}.md >> "log_${book}.txt"
		done
	done
done
#! /usr/bin/perl

system('make');

opendir($DH,'./tests');
while ($file = readdir($DH)){
	if ($file =~ /example/){
		print "input is: $file";
		if ($file =~ /example[1-4]\./){
			system("./sim_main tests/$file 43 > out1.txt");
		} else {
			system("./sim_main tests/$file 80 > out1.txt");
		}
		$file =~ /(example[0-9]*)/;
		$name = $1;
		@out = `diff out1.txt results/$name\.txt`;
		$numOfRows = @out;
#		$line = shift @out;
		if ($numOfRows == 4){
			print "......[Success]\n";
		} else {
			print "			XXXXXX[Failed!]\n";
		}

	} 	
}
closedir($DH);
opendir($DH,'./tests');
while ($file = readdir($DH)){
	if ($file =~ /example/){
		print "input is: $file with -s";
		if ($file =~ /example[1-4]\./){
			system("./sim_main tests/$file 43 -s> out1.txt");
		} else {
			system("./sim_main tests/$file 80 -s> out1.txt");
		}
		$file =~ /(example[0-9]*)/;
		$name = $1 . "-s.txt";
		@out = `diff out1.txt results/$name`;
		$numOfRows = @out;
#		$line = shift @out;
		if ($numOfRows == 4){
			print "......[Success]\n";
		} else {
			print "			XXXXXX[Failed!]\n";
		}
	} 	
}
closedir($DH);
opendir($DH,'./tests');
while ($file = readdir($DH)){

	if ($file =~ /example/){
		print "input is: $file with -f";
		if ($file =~ /example[1-4]\./){
			system("./sim_main tests/$file 43 -f> out1.txt");
		} else {
			system("./sim_main tests/$file 80 -f> out1.txt");
		}		$file =~ /(example[0-9]*)/;
		$name = $1 . "-f.txt";
		@out = `diff out1.txt results/$name`;
		$numOfRows = @out;
#		$line = shift @out;
		if ($numOfRows == 4){
			print "......[Success]\n";
		} else {
			print "			XXXXXX[Failed!]\n";
		}

	} 	
}
closedir($DH);
unlink('out1.txt');
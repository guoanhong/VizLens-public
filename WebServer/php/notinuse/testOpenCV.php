<?php
	echo "Hello";
    error_reporting(E_ALL);
    ini_set("display_errors", 1);
    
    $src = "../testOpenCV/7-crop.jpg";
    $dst = "../testOpenCV/9.jpg";
    
    //$cmd = sprintf("../testOpenCV/OpenCV %s %s > output.txt", $src, $dst);
    
    $cmd = sprintf("./../testOpenCV/HelloWorld > output1.txt");
    
    $result = exec($cmd, $retval);
    printf("Result: '%s'\n\nReturn value: %d\n\n", $result, $retval);
?>
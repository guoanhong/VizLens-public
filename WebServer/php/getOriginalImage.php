<?php
    error_reporting(E_ALL);
    ini_set("display_errors", 1);

    include('_db.php');

    try {
        $dbh = getDatabaseHandle();
    } catch (PDOException $e) {
        echo $e->getMessage();
    }

    if ($dbh) {
    	$workerid = $_REQUEST['workerid'];
    	$session = $_REQUEST['session'];
        $stateid = $_REQUEST['stateid'];
        $task = $_REQUEST['task'];
        
        $sth = $dbh->prepare("SELECT * FROM images_original WHERE session=:session AND stateid=:stateid AND active=1 ORDER BY RANDOM() ASC LIMIT 1");
        $sth->execute(array(':session'=>$session, ':stateid'=>$stateid));

        // Return the results
        $row = $sth->fetch(PDO::FETCH_ASSOC, PDO::FETCH_ORI_NEXT);
            
    	//$retAry = array();
    	//$retAry["image"] = $row["image"];
    	$retAry = $row;

    	$imageid = $row["id"];

    	if($imageid){
    		$session = $row["session"];
            $stateid = $row["stateid"];
            $sth = $dbh->prepare("INSERT INTO workers (imageid,workerid,session,stateid,task) VALUES(:imageid,:workerid,:session,:stateid,:task)");
            $sth->execute(array(':imageid'=>$imageid, ':workerid'=>$workerid, ':session'=>$session, ':stateid'=>$stateid, ':task'=>$task));
    	}
        print(json_encode($retAry));
    } else {
        print("FAILING");
    }
?>
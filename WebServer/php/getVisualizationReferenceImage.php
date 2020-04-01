<?php
    error_reporting(E_ALL);
    ini_set("display_errors", 1);

    include('_db.php');

    $dbh = getDatabaseHandle();

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

        $sth = $dbh->prepare("SELECT * FROM images_reference WHERE session=:session AND stateid=:stateid ORDER BY RANDOM() ASC LIMIT 1");
        $sth->execute(array(':session'=>$session, ':stateid'=>$stateid));

        // Return the results
        $row = $sth->fetch(PDO::FETCH_ASSOC, PDO::FETCH_ORI_NEXT);

        $retAry = $row;
        
        // select individual answers from answers_label
        $sth_label = $dbh->prepare("SELECT button FROM answers_label WHERE session=:session AND stateid=:stateid AND source=:source");
        $sth_label->execute(array(':session'=>$session, ':stateid'=>$stateid, ':source'=>'individual'));
        $row_label = $sth_label->fetchAll();

        // select button count from answers_crop
        $sth_crop = $dbh->prepare("SELECT buttons FROM answers_crop WHERE session=:session AND stateid=:stateid AND source=:source");
        $sth_crop->execute(array(':session'=>$session, ':stateid'=>$stateid, ':source'=>'aggregated'));

        $row_crop = $sth_crop->fetch(PDO::FETCH_ASSOC, PDO::FETCH_ORI_NEXT);

        $buttons_number = $row_crop["buttons"];

        error_log("Label: number of responses is: " . count($row_label));
        error_log("Label: number of buttons is: " . $buttons_number);
        
        // bounding box properties
        // $newx1, $newx2, $newy1, $newy2, $row[$i]['x1'], $row[$i]['y1'], $row[$i]['x2'], $row[$i]['y2']

        $buttonIndex = 0;
        $largestButtonIndex = 0;

        for ($i=0; $i < count($row_label); $i++) {
            if ($row_label[$i]['button'] > $largestButtonIndex) {
                $largestButtonIndex = $row_label[$i]['button'];
            }
        }
        error_log("Largest Button Index: " . $largestButtonIndex);

        if ($largestButtonIndex >= $buttons_number-1) {
            $retAry["alllabeled"] = 1;
        }
        else {
            $retAry["alllabeled"] = 0;
        }

        $retAry["active"] = 1;

        print(json_encode($retAry));
    } else {
        print("FAILING");
    }
?>
<?php
    error_reporting(E_ALL);
    ini_set("display_errors", 1);

    include('_db.php');
    include('checkCropAnswers.php');

    if( !isset($_REQUEST['answer']) ) {
        print 'no message';
    }
    if( !isset($_REQUEST['session']) ) {
        print 'no session';
    }
    if( !isset($_REQUEST['stateid']) ) {
        print 'no stateid';
    }

    if( isset($_REQUEST['description']) && isset($_REQUEST['session']) && isset($_REQUEST['stateid'])) {
        try {
            $dbh = getDatabaseHandle();
        } catch( PDOException $e ) {
            echo $e->getMessage();
        }

        if( $dbh ) {
            $session  = $_REQUEST['session'];
            $stateid  = $_REQUEST['stateid'];
            $complete  = $_REQUEST['complete'];
            $clear  = $_REQUEST['clear'];
            $x1  = $_REQUEST['x1'];
            $y1  = $_REQUEST['y1'];
            $x2  = $_REQUEST['x2'];
            $y2  = $_REQUEST['y2'];
            $buttons = $_REQUEST['buttons'];
            $description = $_REQUEST['description'];
            $source = "individual";
            //print($ans . " | " . $session . " | " . $wIdx . " | " . $hIdx);

            $query = $dbh->prepare("INSERT INTO answers_crop (session,stateid,complete,clear,x1,y1,x2,y2,buttons,description,source) VALUES(:session,:stateid,:complete,:clear,:x1,:y1,:x2,:y2,:buttons,:description,:source)");
            $query->execute(array(':session'=>$session, ':stateid'=>$stateid, ':complete'=>$complete, ':clear'=>$clear, ':x1'=>$x1, ':y1'=>$y1, ':x2'=>$x2, ':y2'=>$y2, ':buttons'=>$buttons, ':description'=>$description, ':source'=>$source));

            $workerid = $_REQUEST['workerid'];
            $query = $dbh->prepare("UPDATE workers SET finishtime=CURRENT_TIMESTAMP WHERE workerid=? AND session=? AND stateid=?");
            $query->execute(array($workerid,$session,$stateid));

            checkCropAnswers($dbh, $session, $stateid);

            $query = $dbh->prepare("INSERT INTO mturk_hits (session,stateid,task,info) VALUES(:session,:stateid,:task,:info)");
            error_log($workerid . ' completed');
            $query->execute(array(':session'=>$session, ':stateid'=>$stateid, ':task'=>'crop', ':info'=>$workerid . ' completed'));

            print("SUCCESS");
        }
        else {
            print("FAILING");
        }
    }
?>
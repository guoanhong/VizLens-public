<?php
    error_reporting(E_ALL);
    ini_set("display_errors", 1);

    include('_db.php');
    include('checkLabelAnswers.php');

    if( !isset($_REQUEST['answer']) ) {
        print 'no message';
    }
    if( !isset($_REQUEST['session']) ) {
        print 'no session';
    }
    if( !isset($_REQUEST['stateid']) ) {
        print 'no stateid';
    }

    if( isset($_REQUEST['label']) && isset($_REQUEST['session']) && isset($_REQUEST['stateid']) ) {
        try {
            $dbh = getDatabaseHandle();
        } catch( PDOException $e ) {
            echo $e->getMessage();
        }

        if( $dbh ) {
            $session  = $_REQUEST['session'];
            $stateid  = $_REQUEST['stateid'];
            $x1  = $_REQUEST['x1'];
            $y1  = $_REQUEST['y1'];
            $x2  = $_REQUEST['x2'];
            $y2  = $_REQUEST['y2'];
            $label = $_REQUEST['label'];
            $workerid = $_REQUEST['workerid'];
            // $source = "individual";
            //print($ans . " | " . $session . " | " . $wIdx . " | " . $hIdx);

            insertLabelAnswers($dbh, $session, $stateid, $x1, $y1, $x2, $y2, $label, $workerid);

            $query = $dbh->prepare("UPDATE workers SET finishtime=CURRENT_TIMESTAMP WHERE workerid=? AND session=? AND stateid=?");
            $query->execute(array($workerid,$session,$stateid));

            $query = $dbh->prepare("INSERT INTO mturk_hits (session,stateid,task,info) VALUES(:session,:stateid,:task,:info)");
            $query->execute(array(':session'=>$session, ':stateid'=>$stateid, ':task'=>'label', ':info'=>$workerid . ' completed'));

            print("SUCCESS");
        }
        else {
            print("FAILING");
        }
    }

    if( isset($_REQUEST['alllabeled']) && isset($_REQUEST['session']) && isset($_REQUEST['stateid']) ) {
        try {
            $dbh = getDatabaseHandle();
        } catch( PDOException $e ) {
            echo $e->getMessage();
        }

        if( $dbh ) {
            $session  = $_REQUEST['session'];
            $stateid  = $_REQUEST['stateid'];
            $alllabeled = $_REQUEST['alllabeled'];
            $workerid = $_REQUEST['workerid'];

            insertLabelAnswersCompleted($dbh, $session, $stateid, $workerid);

            $workerid = $_REQUEST['workerid'];
            $query = $dbh->prepare("UPDATE workers SET finishtime=CURRENT_TIMESTAMP WHERE workerid=? AND session=? AND stateid=?");
            $query->execute(array($workerid,$session,$stateid));

            $query = $dbh->prepare("INSERT INTO mturk_hits (session,stateid,task,info) VALUES(:session,:stateid,:task,:info)");
            $query->execute(array(':session'=>$session, ':stateid'=>$stateid, ':task'=>'label', ':info'=>$workerid . ' completed'));

            print("SUCCESS");
        }
        else {
            print("FAILING");
        }
    }
?>

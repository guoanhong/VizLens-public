<?php
    error_reporting(E_ALL);
    ini_set("display_errors", 1);

    include('_db.php');

    try {
        $dbh = getDatabaseHandle();
    } catch( PDOException $e ) {
        echo $e->getMessage();
    }

    if( $dbh ) {
        $userid = $_REQUEST["userid"];

        $sth_video = $dbh->prepare("SELECT id, session, stateid, feedback FROM images_video WHERE userid=:userid AND imageactive=:imageactive AND feedbackactive=:feedbackactive ORDER BY id DESC");
        $sth_video->execute(array(':userid'=>$userid, ':imageactive'=>0, ':feedbackactive'=>1));

        $row = $sth_video->fetch(PDO::FETCH_ASSOC, PDO::FETCH_ORI_NEXT);

        $id = $row["id"];
        $session = $row["session"];
        $stateid = $row["stateid"];
        $feedback = $row["feedback"];

        // Comment out for testing without CVServer
        $sth_feedback = $dbh->prepare("UPDATE images_video SET feedbackactive=:feedbackactive,feedbacktime=current_timestamp WHERE id=:id");
        $sth_feedback->execute(array(':feedbackactive'=>0, ':id'=>$id));

        print($stateid.'|'.$feedback);
    }
    else {
        print("FAILING");
    }
?>
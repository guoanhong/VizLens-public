<!-- active property not used -->
<?php
    error_reporting(E_ALL);
    ini_set("display_errors", 1);

    include('_db.php');

    echo('here');
    // Insert pointer into DB
    try {
        $dbh = getDatabaseHandle();
    } catch( PDOException $e ) {
        echo $e->getMessage();
    }

    echo('here2');

    if( $dbh ) {
        $session = $_REQUEST["session"];
        //$image = $_REQUEST["image"];
        echo($session);

        //$sth = $dbh->prepare("UPDATE images SET active=0 WHERE session=:session AND image=:image)");
        $sth = $dbh->prepare("UPDATE images SET active=0 WHERE session=:session");
        //$sth->execute(array(':session'=>$session, ':image'=>$image));
        $sth->execute(array(':session'=>$session));

        print("SUCCESS");
    }
    else {
        print("FAILING");
    }
?>
<?php
    error_reporting(E_ALL);
    ini_set("display_errors", 1);

    include('setReference.php');

    function checkCropAnswers($dbh, $session, $stateid){
    	$query = $dbh->prepare("SELECT complete, clear, x1, y1, x2, y2, buttons, description FROM answers_crop WHERE session=:session AND stateid=:stateid AND source=:source");
        $query->execute(array(':session'=>$session, ':stateid'=>$stateid, ':source'=>'individual'));
        $row = $query->fetchAll();

        $query = $dbh->prepare("SELECT complete, clear, x1, y1, x2, y2, buttons, description FROM answers_crop WHERE session=:session AND stateid=:stateid AND source=:source");
        $query->execute(array(':session'=>$session, ':stateid'=>$stateid, ':source'=>'aggregated'));
        $row_aggregated = $query->fetchAll();

        error_log("Crop: number of responses is: " . count($row));
        error_log("Crop: number of aggregated responses is: " . count($row_aggregated));

        //error_log(json_encode($row_aggregated), 0);
        //count($row_aggregated) == 0
        if (count($row_aggregated) == 0) {
            $complete_responses = 0;
            $clear_responses = 0;
            $incomplete_responses = 0;
            $unclear_responses = 0;

            $x1_array = array();
            $y1_array = array();
            $x2_array = array();
            $y2_array = array();
            $buttons_array = array();
            $description_array = array();

            for ($i=0; $i < count($row); $i++) { 
                $complete_responses += ($row[$i]['complete'] == 1);
                $incomplete_responses += ($row[$i]['complete'] == 2);
                $clear_responses += ($row[$i]['clear'] == 1);
                $unclear_responses += ($row[$i]['clear'] == 2);

                if (($row[$i]['complete'] == 1) && ($row[$i]['clear'] == 1)) {
                    error_log("Complete and Clear");
                    array_push($x1_array, $row[$i]['x1']);
                    array_push($y1_array, $row[$i]['y1']);
                    array_push($x2_array, $row[$i]['x2']);
                    array_push($y2_array, $row[$i]['y2']);
                    array_push($buttons_array, $row[$i]['buttons']);
                    array_push($description_array, $row[$i]['description']);
                }
            }

            error_log($complete_responses.$incomplete_responses.$clear_responses.$unclear_responses);

            error_log(implode(",",$x1_array).' '.implode(",",$y1_array).' '.implode(",",$x2_array).' '.implode(",",$y2_array).' '.implode(",",$buttons_array).' '.implode(",",$description_array));

            // Tempororaly disable majority voting
            if ($complete_responses >= 1 && $clear_responses >= 1) {
                //complete and clear
                error_log("Complete and Clear");

                //calculate answers and write
                $query = $dbh->prepare("INSERT INTO answers_crop (session,stateid,complete,clear,x1,y1,x2,y2,buttons,description,source) VALUES(:session,:stateid,:complete,:clear,:x1,:y1,:x2,:y2,:buttons,:description,:source)");
                $query->execute(array(':session'=>$session, ':stateid'=>$stateid, ':complete'=>1, ':clear'=>1, ':x1'=>round(array_sum($x1_array)/count($x1_array)), ':y1'=>round(array_sum($y1_array)/count($y1_array)), ':x2'=>round(array_sum($x2_array)/count($x2_array)), ':y2'=>round(array_sum($y2_array)/count($y2_array)), ':buttons'=>round(array_sum($buttons_array)/count($buttons_array)), ':description'=>findLongestStringFromArray($description_array), ':source'=>'aggregated'));

                setReference($dbh, $session, $stateid);

                $query = $dbh->prepare("UPDATE images_original SET active=:active WHERE session=:session AND stateid=:stateid AND active=1");
                $query->execute(array(':active'=>0, ':session'=>$session, ':stateid'=>$stateid));

                if (strlen($stateid)==1) {
                    $stateid = '0' . $stateid;
                }
                $image_name =  $stateid . '-crop.jpg';

                // feedback
                $query = $dbh->prepare("UPDATE objects SET status=:status WHERE session=:session AND stateid=:stateid AND status!='deprecated'");
                $query->execute(array(':status'=>'processing', ':session'=>$session, ':stateid'=>$stateid));
                // complete and clear

                $query = $dbh->prepare("UPDATE objects SET image=:image WHERE session=:session AND stateid=:stateid");
                $query->execute(array(':image'=>$image_name, ':session'=>$session, ':stateid'=>$stateid));
            }

            // if ($complete_responses >= 3 && $clear_responses >= 3) {
            //     //complete and clear
            //     error_log("Complete and Clear");

            //     //calculate answers and write
            //     $query = $dbh->prepare("INSERT INTO answers_crop (session,stateid,complete,clear,x1,y1,x2,y2,buttons,description,source) VALUES(:session,:stateid,:complete,:clear,:x1,:y1,:x2,:y2,:buttons,:description,:source)");
            //     $query->execute(array(':session'=>$session, ':stateid'=>$stateid, ':complete'=>1, ':clear'=>1, ':x1'=>round(array_sum($x1_array)/count($x1_array)), ':y1'=>round(array_sum($y1_array)/count($y1_array)), ':x2'=>round(array_sum($x2_array)/count($x2_array)), ':y2'=>round(array_sum($y2_array)/count($y2_array)), ':buttons'=>round(array_sum($buttons_array)/count($buttons_array)), ':description'=>findLongestStringFromArray($description_array), ':source'=>'aggregated'));

            //     setReference($dbh, $session, $stateid);

            //     $query = $dbh->prepare("UPDATE images_original SET active=:active WHERE session=:session AND stateid=:stateid AND active=1");
            //     $query->execute(array(':active'=>0, ':session'=>$session, ':stateid'=>$stateid));

            //     if (strlen($stateid)==1) {
            //         $stateid = '0' . $stateid;
            //     }
            //     $image_name =  $stateid . '-crop.jpg';

            //     // feedback
            //     $query = $dbh->prepare("UPDATE objects SET status=:status WHERE session=:session AND stateid=:stateid AND status!='deprecated'");
            //     $query->execute(array(':status'=>'processing', ':session'=>$session, ':stateid'=>$stateid));
            //     // complete and clear

            //     $query = $dbh->prepare("UPDATE objects SET image=:image WHERE session=:session AND stateid=:stateid");
            //     $query->execute(array(':image'=>$image_name, ':session'=>$session, ':stateid'=>$stateid));
            // }
            // elseif ($complete_responses >= 3 && $unclear_responses >= 3) {
            //     //complete but unclear
            //     error_log("Complete but Unclear");
            //     $query = $dbh->prepare("INSERT INTO answers_crop (session,stateid,complete,clear,source) VALUES(:session,:stateid,:complete,:clear,:source)");
            //     $query->execute(array(':session'=>$session, ':stateid'=>$stateid, ':complete'=>1, ':clear'=>2, ':source'=>'aggregated'));
                
            //     // feedback
            //     $query = $dbh->prepare("UPDATE objects SET status=:status WHERE session=:session AND stateid=:stateid");
            //     $query->execute(array(':status'=>'complete but not clear, retake', ':session'=>$session, ':stateid'=>$stateid));
            // }
            // elseif ($incomplete_responses >= 3 && $clear_responses >= 3) {
            //     //incomplete but clear
            //     error_log("Incomplete but Clear");
            //     $query = $dbh->prepare("INSERT INTO answers_crop (session,complete,clear,source) VALUES(:session,:complete,:clear,:source)");
            //     $query->execute(array(':session'=>$session, ':complete'=>2,':clear'=>1, ':source'=>'aggregated'));
                
            //     // feedback
            //     $query = $dbh->prepare("UPDATE objects SET status=:status WHERE session=:session AND stateid=:stateid");
            //     $query->execute(array(':status'=>'clear but not complete, retake', ':session'=>$session, ':stateid'=>$stateid));
            // }
            // elseif ($incomplete_responses >= 3 && $unclear_responses >= 3) {
            //     //incomplete and unclear
            //     error_log("Incomplete and Unclear");
            //     $query = $dbh->prepare("INSERT INTO answers_crop (session,complete,clear,source) VALUES(:session,:complete,:clear,:source)");
            //     $query->execute(array(':session'=>$session, ':complete'=>2, ':clear'=>2, ':source'=>'aggregated'));
                
            //     // feedback
            //     $query = $dbh->prepare("UPDATE objects SET status=:status WHERE session=:session AND stateid=:stateid");
            //     $query->execute(array(':status'=>'not complete and not clear, retake', ':session'=>$session, ':stateid'=>$stateid));
            // }
        }
	}

    function findLongestStringFromArray($array = array()) {
        if(!empty($array)){
            $lengths = array_map('strlen', $array);
            $maxLength = max($lengths);
            $key = array_search($maxLength, $lengths);
            return $array[$key];
        }
    }
?>
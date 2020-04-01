<?php
    error_reporting(E_ALL);
    ini_set("display_errors", 1);

    function insertLabelAnswers($dbh, $session, $stateid, $newx1, $newy1, $newx2, $newy2, $newlabel, $newworkerid){
        // select individual answers from answers_label
    	$query = $dbh->prepare("SELECT x1, y1, x2, y2, label, button FROM answers_label WHERE session=:session AND stateid=:stateid AND source=:source");
        $query->execute(array(':session'=>$session, ':stateid'=>$stateid, ':source'=>'individual'));
        $row = $query->fetchAll();

        // select button count from answers_crop
        $query = $dbh->prepare("SELECT buttons FROM answers_crop WHERE session=:session AND stateid=:stateid AND source=:source");
        $query->execute(array(':session'=>$session, ':stateid'=>$stateid, ':source'=>'aggregated'));
        $row_crop = $query->fetch(PDO::FETCH_ASSOC, PDO::FETCH_ORI_NEXT);
        $buttons_number = $row_crop["buttons"];

        error_log("Label: number of responses is: " . count($row));
        error_log("Label: number of buttons is: " . $buttons_number);

        // bounding box properties
        // $newx1, $newx2, $newy1, $newy2, $row[$i]['x1'], $row[$i]['y1'], $row[$i]['x2'], $row[$i]['y2']

        $buttonIndex = 0;
        $largestButtonIndex = 0;

        for ($i=0; $i < count($row); $i++) {
            if ($row[$i]['button'] > $largestButtonIndex) {
                $largestButtonIndex = $row[$i]['button'];
            }
        }
        error_log("Largest Button Index: " . $largestButtonIndex);

        for ($i=0; $i < count($row); $i++) { 
            // detect whether the two bounding boxes are overlapping with each other
            $overlap = (($newx1 + $newx2) > 2 * $row[$i]['x1']) && (($newx1 + $newx2) < 2 * $row[$i]['x2']) && (($row[$i]['y1'] + $row[$i]['y2']) > 2 * $newy1) && (($row[$i]['y1'] + $row[$i]['y2']) < 2 * $newy2);

            if ($overlap) {
                $buttonIndex = $row[$i]['button'];
                break;
            }
            else {
                $buttonIndex = $largestButtonIndex + 1;
            }
        }
        
        error_log("Current Button Index: " . $buttonIndex);

        // insert new label answer to database
        $query = $dbh->prepare("INSERT INTO answers_label (session,stateid,x1,y1,x2,y2,label,button,source,workerid,active) VALUES(:session,:stateid,:x1,:y1,:x2,:y2,:label,:button,:source,:workerid,:active)");
        $query->execute(array(':session'=>$session, ':stateid'=>$stateid, ':x1'=>$newx1, ':y1'=>$newy1, ':x2'=>$newx2, ':y2'=>$newy2, ':label'=>$newlabel, ':button'=>$buttonIndex, ':source'=>'individual', ':workerid'=>$newworkerid, ':active'=>1));

        // if all the hits are used up, add more according to the buttons that are not labeled
        if ($buttons_number * 1.5 <= count($row)) {
            // POST more HITs
            if ($buttons_number - 1 > $largestButtonIndex) {
                $addHITs = round(($buttons_number - $largestButtonIndex) * 1.5);
            }
            else {
                $addHITs = 2;
            }

            $command = escapeshellcmd('python ../mturk/createExternalHIT-label.py '.$session.' '.$stateid.' '.$addHITs);
            $output = shell_exec($command);
            print($output);
            $sth_HITs = $dbh->prepare("INSERT INTO mturk_hits (session,stateid,task,info) VALUES(:session,:stateid,:task,:info)");
            $sth_HITs->execute(array(':session'=>$session, ':stateid'=>$stateid, ':task'=>'label', ':info'=>'fire additional ' . $HITs . ' hits ' . $output));
        }
	}

    function aggregateLabelAnswers($dbh, $session, $stateid){
        // select aggregated answers from answers_label
        $query = $dbh->prepare("SELECT x1, y1, x2, y2, label FROM object_buttons WHERE session=:session AND stateid=:stateid");
        $query->execute(array(':session'=>$session, ':stateid'=>$stateid));
        $row_aggregated = $query->fetchAll();

        // select button count from answers_crop
        $query = $dbh->prepare("SELECT buttons FROM answers_crop WHERE session=:session AND stateid=:stateid AND source=:source");
        $query->execute(array(':session'=>$session, ':stateid'=>$stateid, ':source'=>'aggregated'));
        $row_crop = $query->fetch(PDO::FETCH_ASSOC, PDO::FETCH_ORI_NEXT);
        $buttons_number = $row_crop["buttons"];

        // error_log("Label: number of responses is: " . count($row));
        error_log("Label: number of aggregated responses is: " . count($row_aggregated));
        error_log("Label: number of buttons is: " . $buttons_number);

        if (count($row_aggregated) == 0) {
            for ($i=0; $i <= $buttons_number-1; $i++) { 
                // select individual answers from answers_label
                $query = $dbh->prepare("SELECT x1, y1, x2, y2, label FROM answers_label WHERE session=:session AND stateid=:stateid AND source=:source AND button=:button");
                $query->execute(array(':session'=>$session, ':stateid'=>$stateid, ':source'=>'individual', ':button'=>$i));
                $row = $query->fetchAll();

                $x1_array = array();
                $y1_array = array();
                $x2_array = array();
                $y2_array = array();
                $label_array = array();

                for ($j=0; $j < count($row); $j++) { 
                    array_push($x1_array, $row[$j]['x1']);
                    array_push($y1_array, $row[$j]['y1']);
                    array_push($x2_array, $row[$j]['x2']);
                    array_push($y2_array, $row[$j]['y2']);
                    array_push($label_array, $row[$j]['label']);
                }
                error_log(implode(",",$x1_array).' '.implode(",",$y1_array).' '.implode(",",$x2_array).' '.implode(",",$y2_array).' '.implode(",",$label_array));

                $x1_aggregated = array_sum($x1_array)/count($row);
                $y1_aggregated = array_sum($y1_array)/count($row);
                $x2_aggregated = array_sum($x2_array)/count($row);
                $y2_aggregated = array_sum($y2_array)/count($row);
                // calculate $label_aggregated
                $label_aggregated = findLongestStringFromArray($label_array);
                
                $query = $dbh->prepare("INSERT INTO object_buttons (session,stateid,x1,y1,x2,y2,label) VALUES(:session,:stateid,:x1,:y1,:x2,:y2,:label)");
                $query->execute(array(':session'=>$session, ':stateid'=>$stateid, ':x1'=>$x1_aggregated, ':y1'=>$y1_aggregated, ':x2'=>$x2_aggregated, ':y2'=>$y2_aggregated, ':label'=>$label_aggregated));
            }

            $query = $dbh->prepare("UPDATE images_reference SET active=:active WHERE session=:session AND stateid=:stateid");
            $query->execute(array(':active'=>0, ':session'=>$session, ':stateid'=>$stateid));

            //feedback: image is ready
            $query = $dbh->prepare("UPDATE objects SET status=:status WHERE session=:session AND stateid=:stateid AND status!='deprecated'");
            $query->execute(array(':status'=>'readytoread', ':session'=>$session, ':stateid'=>$stateid));
        }
    }

    function aggregateLabels($label_array){
        $words = file_get_contents('../word_lists/Microwave.txt');
        $words_list = explode(", ", $words);
        $unique_words_list = array_unique($words_list);

        $label_word_list = array();

        for ($i=0; $i < count($label_array); $i++) { 
            $label_array[$i] = str_replace("/"," ",$label_array[$i]);
            $label_array[$i] = str_replace("-"," ",$label_array[$i]);
            $label_array[$i] = str_replace("_"," ",$label_array[$i]);
            $label_array[$i] = str_replace("("," ",$label_array[$i]);
            $label_array[$i] = str_replace(")"," ",$label_array[$i]);
            $label_array[$i] = str_replace("."," ",$label_array[$i]);
            $label_array[$i] = str_replace("+"," ",$label_array[$i]);
            $label_array[$i] = preg_replace('!\s+!', ' ', $label_array[$i]);
            $label_word_list[$i] = explode(" ", $label_array[$i]);
        }

        // for ($i=0; $i < count($label_word_list); $i++) { 
        //     error_log(json_encode($label_word_list[$i]));
        // }

        $label_word_list_score = array_fill(0, count($label_word_list), 0);

        // if all agree, or only one response
        if (count(array_unique($label_array)) == 1) {
            $label_aggregated = $label_array[0];
        }
        // compare with word list and choose the one with the highest score
        else {
            for ($i=0; $i < count($label_word_list); $i++) { 
                for ($j=0; $j < count($label_word_list[$i]); $j++) { 
                    // error_log(json_encode($label_word_list[$i][$j]));
                    for ($k=0; $k < count($unique_words_list); $k++) { 
                        if ($label_word_list[$i][$j] == $unique_words_list[$k]) {
                            $label_word_list_score[$i]++;
                        }
                    }
                }
                preg_match_all('!\d+!', $label_array[$i], $matches);
                $label_word_list_score[$i] += intval($matches[0]);

                error_log("Numbers count: " . intval($matches[0]));
            }

            for ($i=0; $i < count($label_word_list_score); $i++) {
                error_log(json_encode($label_word_list_score[$i]));
            }

            $maxs_score = array_keys($label_word_list_score, max($label_word_list_score));
            error_log($maxs_score[0]);
            if ($maxs_score[0] > 0) {
                $label_aggregated = $label_array[$maxs_score[0]];
            }
            elseif ($maxs_score[0] == 0) {
                //if all different -> majority, if different -> latest
                if (count(array_unique($label_array)) == count($label_array)) {
                    $label_aggregated = $label_array[count($label_array) - 1];
                }
                elseif (count(array_unique($label_array)) < count($label_array)) {
                    error_log("Test message");
                    $label_frequency = array_count_values($label_array);
                    $maxs_frequency = array_keys($label_frequency, max($label_frequency));
                    error_log($maxs_frequency[0]);
                    $label_aggregated = $maxs_frequency[0];
                }
            }
        }

        return $label_aggregated;
    }

    function findLongestStringFromArray($array = array()) {
        if(!empty($array)){
            $lengths = array_map('strlen', $array);
            $maxLength = max($lengths);
            $key = array_search($maxLength, $lengths);
            return $array[$key];
        }
    }

    function insertLabelAnswersCompleted($dbh, $session, $stateid, $workerid){
        $query = $dbh->prepare("INSERT INTO answers_label (session,stateid,workerid,active) VALUES(:session,:stateid,:workerid,:active)");
        $query->execute(array(':session'=>$session, ':stateid'=>$stateid, ':workerid'=>$workerid, ':active'=>0));

        // determine whether more than 3 workers agree with each other
        $query = $dbh->prepare("SELECT * FROM answers_label WHERE session=:session AND stateid=:stateid AND active=:active");
        $query->execute(array(':session'=>$session, ':stateid'=>$stateid, ':active'=>0));
        $row = $query->fetchAll();

        // Tempororaly disable majority voting
        if (count($row) >= 1){
        // if (count($row) >= 3){
            // start to aggregate answers
            aggregateLabelAnswers($dbh, $session, $stateid);

            $query = $dbh->prepare("UPDATE images_reference SET active=:active WHERE session=:session AND stateid=:stateid");
            $query->execute(array(':active'=>0, ':session'=>$session, ':stateid'=>$stateid));
        }
    }
?>
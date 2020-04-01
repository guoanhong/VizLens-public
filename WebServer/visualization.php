<?php session_start(); ?>
<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.01 Transitional//EN" "http://www.w3.org/TR/html4/loose.dtd">
<html>
	<head>
		<title>Visualization</title>
		<!-- Libraries -->
		<script src="https://ajax.googleapis.com/ajax/libs/jquery/1.7.2/jquery.min.js" type="text/javascript"></script>
		<!--script type="text/javascript" src="https://ajax.googleapis.com/ajax/libs/swfobject/2.2/swfobject.js"></script-->
		<script src="scripts/gup.js" type="text/javascript"></script>
		<link rel="stylesheet" type="text/css" href="css/turk.css"></link>
		<link rel="stylesheet" href="css/legion.css" />
		<script src="scripts/vars.js"></script>
		<script src="scripts/legion.js"></script>

		<!-- Scripts -->
		<script src="scripts/visualizationPage.js" type="text/javascript"></script>

		<!-- Style -->
		<link rel="stylesheet" type="text/css" href="css/style.css"></link>
		<link rel="stylesheet" type="text/css" href="css/overlay.css"></link>

		<!-- JCrop -->
		<script src="jcrop/js/jquery.Jcrop.min.js" type="text/javascript"></script>
		<script src="scripts/region.js" type="text/javascript"></script>

		<!-- Overlay -->
		<script src="scripts/visualization_overlay.js" type="text/javascript"></script>
	</head>
	<body>
		<div id="main-interface">
		    <div id="outer-container-label">
				<div id="header">
					<div id="title">
						Visualization
					</div>
					<br>
					<div align="center" id="appliancename">
						Appliance Name: <p id="interface-name" style="display:inline">Appliance</p>
					</div>
				</div>

				<div id="arrow_wrapper_top"></div>
				<div id="container">
					<div id="instr-area"></div>
					<br/>
					<div id="entry_header"></div>
					<div id="image_wrapper"></div>
				</div>
				<div id="arrow_wrapper_bottom"></div>

				<div id="submit-div">
					<form id="submit-form">
			  			<input type="hidden" name="money" id="money_field">
						<input type="hidden" name="assignmentId" id="submit-assignmentId">
					</form>
				</div>
		    </div>

		    <div id="all-labeled" style="text-align: center;">
		    	<strong><h2>We think all the buttons are already labeled. Is that true? 
					<input id="alllabeled" type="radio" name="alllabeled" value="yes"> True  
					<input id="notalllabeled" type="radio" name="alllabeled" value="no"> False 
				</h2></strong>
		    </div>

		    <div id="active" style="text-align:center;">
		    	<strong><h2>The image is no longer active.</h2></strong>
		    </div>

		    <!-- Label box -->
		    <div id="label-wrapper">
				<p><b>Please describe the area you selected:</b></p>
				<textarea type="textbox" id="labelText" spellcheck="true"></textarea>
		    </div>
		</div>

		<div id="sidebar"></div>
		<div id="instructions"></div>
		<p id='legion-money' style="display:none;">.15</p>
	</body>
</html>
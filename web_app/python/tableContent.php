
<!DOCTYPE html>
<html lang="en">
	 <head>
				<meta content="text/html;charset=utf-8" http-equiv="Content-Type">
				<meta content="utf-8" http-equiv="encoding">
      </head>
    <body>

<?php
		// require '../db/logging.php';
		if(isset($_POST['uni_coeff'])) {
			$csv_data = $_POST['uni_coeff'];
			if (file_put_contents("uni_coeff.json", $csv_data))
			      echo "JSON uni_variate file created successfully...";
			else
			      echo "Oops! Error creating json file...";
			$uni_result = shell_exec('python multiCoeff.py uni_coeff.json uni');
		}else if(isset($_POST['multi_coeff'])){
			$json = $_POST['multi_coeff'];
			if (file_put_contents("multi_coeff.json", $json))
						echo "JSON multi_variate file created successfully...";
			else
						echo "Oops! Error creating json file...";
			$multi_result = shell_exec('python multiCoeff.py multi_coeff.json multi');
		}
?>
    </body>
</html>

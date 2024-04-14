<?PHP
	//This is a fragment of code of automatic quque managemement/registration, 
	//can be tested by this URL: https://pass.allmynotes.org/info?target=loading-of-all
	include_once("core_pass.php");
	include_once("core_mail.php");

//	if (!is_debug_session())
//		exit; //this page is for internal use only

	$this_page_url = GetThisUrl();
	$ip = get_user_IP_address();

	$b_called_by_search_engine = IsPageCalledBySearchEngine();
	$b_heavy_cpu_load = IsServerUnderHeavyLoad();
	$referer = getenv("HTTP_REFERER");

	$pass_points_array = pass_get_pass_points_array();
	$cars_queue_array = LoadConfigData("cars_border");
	
	$target_ppage = GetParam('target');

	if ($target_ppage == 'whole-line')
	{
		//////////////////////////////////////////////////
		//
		//
		//	Show all cars in the line
		//
		//
		//////////////////////////////////////////////////

		draw_pass_document_top("Статус черги на пропускному пункті", "TlbrWholeLineStatusBtn", "Статус черги на пропускному пункті, скільки вантажівок чекає на кордоні", "Статус черги на пропускному пункті");

		$pass_point_id = GetParam('pass_point_id');
		if ($pass_point_id == "")
			$pass_point_id = array_key_first($pass_points_array); //to-do: use cookies....
		

		echo '<h1>Статус черги по пропускному пункту ' . $pass_points_array[$pass_point_id] . '</h1>
			  <br />';

		$in_car_n = 1;
		$in_cars_list_html = "";
		$out_car_n = 1;
		$out_cars_list_html = "";

		foreach ($cars_queue_array as $car_plate_num => $car_pass_info)
			if ($car_pass_info[DATA_KEY_PASS_POINT_ID] == $pass_point_id)
			{
				$car_passing_eta = pass_compute_car_eta($cars_queue_array, $car_pass_info);

				if ($car_pass_info[DATA_KEY_B_ENRENCE])
				{
					$in_cars_list_html .= "<p>" . $in_car_n . '. <b><a href="' . URL_homepage . '/?plate=' . $car_pass_info[DATA_KEY_CAR_PLATE] .'">' . $car_pass_info[DATA_KEY_CAR_PLATE] . "</a></b> - Expected entry to Ukraine " . date('j.m.Y G:i', $car_passing_eta['queue_passing_date']) . " - in " . $car_passing_eta['queue_passing_eta_str'] . "</p>";
					$in_car_n ++;
				} else
				{
					$out_cars_list_html .= "<p>" . $out_car_n . '. <b><a href="' . URL_homepage . '/?plate=' . $car_pass_info[DATA_KEY_CAR_PLATE] .'">' . $car_pass_info[DATA_KEY_CAR_PLATE] . "</a></b> - Expected exit from Ukraine " . date('j.m.Y G:i', $car_passing_eta['queue_passing_date']) . " - in " . $car_passing_eta['queue_passing_eta_str'] . "</p>";
					$out_car_n ++;
				}
			}

		echo '<h2>Список на <b>в\'їзд</b> в Україну:</h2>
			  <br />';
		if ($in_car_n == 1)
			echo '<p>--- Черга <b>пуста</b> ---</p>';
		else
			echo $in_cars_list_html;

		echo '<br />
			 ';

		echo '<h2>Список на <b>виїзд</b> з України:</h2>
			  <br />';
		if ($out_car_n == 1)
			echo '<p>--- Черга <b>пуста</b> ---</p>';
		else
			echo $out_cars_list_html;

		echo '<br />
			  <p>Кількість машин з обох напрямків: ' . ($in_car_n + $out_car_n - 2) . '</p>
			  <br />
			  <br />
			 ';

		draw_document_end(false);
	} else
	if ($target_ppage == 'missed-in-24')
	{
		//////////////////////////////////////////////////
		//
		//
		//		Show all cars that have failed to appear 
		//	in time during last 24 hours
		//
		//
		//////////////////////////////////////////////////

		draw_pass_document_top("Статус черги на пропускному пункті", "TlbrMissedStatusBtn", "Статус черги на пропускному пункті, скільки вантажівок чекає на кордоні", "Статус черги на пропускному пункті");

		echo '
			<h1>Список автомобілів які були записані в електронну чергу та не з\'явилися [.......назва пункту.......]</h1>
			<br />
			<p>Сайт у розробці, скоро доробимо...</p>
			<br />
			<br />
			';

		draw_document_end(false);
	} else
	if ($target_ppage == 'loading-of-all')
	{
		//////////////////////////////////////////////////
		//
		//
		//	Show loading of all pass points
		//
		//
		//////////////////////////////////////////////////

		draw_pass_document_top("Queue Status at Checkpoint", "TlbrWholeLineStatusBtn", "Queue Status at Checkpoint, how many trucks are waiting at the border", "Queue Status at Checkpoint");

		echo '<h1>Queue Status by Checkpoint ' . $pass_points_array[$pass_point_id] . '</h1><br />';
		
		foreach ($pass_points_array as $point_id => $point_name)
		{
			$b_in_cars_num = 0;
			$b_out_cars_num = 0;
			foreach ($cars_queue_array as $car_plate_num => $car_pass_info)
				if ($car_pass_info[DATA_KEY_PASS_POINT_ID] == $point_id)
				{
					if ($car_pass_info[DATA_KEY_B_ENRENCE])
						$b_in_cars_num ++;
					else
						$b_out_cars_num ++;
				}

			echo "<p><a href=\"" . URL_Pass_whole_line . "&pass_point_id=" . $point_id . "\">{$point_name}</a>: <b>{$b_out_cars_num}</b> виїжджають з України, <b>{$b_in_cars_num}</b> вїжджають в Україну.</p>";
		}

		echo '<br />
			  <br />
			';

		draw_document_end(false);
	} else
	{
		//real homepage
		include_once("error404.php");
	}
?>

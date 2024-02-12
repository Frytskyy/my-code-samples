<?PHP
	//	This is a part of the Cyberium codename project. Part of AllMyNotes Organizer source.
	//	Copyright (c) 2000-2024 Volodymyr Frytskyy. All rights reserved.
	//
	//	THIS SOURCE FILE IS THE PROPERTY OF VOLODYMYR FRYTSKYY AND IS NOT TO BE
	//	RE-DISTRIBUTED OR USED BY ANY MEANS WHATSOEVER WITHOUT THE EXPRESSED WRITTEN
	//	CONSENT OF VOLODYMYR FRYTSKYY.
	//
	//	CONTACT INFORMATION: support@vladonai.com, www.vladonai.com, www.allmynotes.org
	//	License: https://www.vladonai.com/allmynotes-organizer-license-agreement

	//Cron sample to run this script: 
	//	curl --silent -u cp:adonai-eyes 'https://www.my_domain.com/cp/cp_ext_sites_crawler.php?task=task_crawler_2&start=ref&cron=true' > /dev/null

	include_once($_SERVER['DOCUMENT_ROOT'] . "/core_l.php");
	include_once($_SERVER['DOCUMENT_ROOT'] . "/core_log.php");
	include_once($_SERVER['DOCUMENT_ROOT'] . "/core_utils.php");
	include_once($_SERVER['DOCUMENT_ROOT'] . "/core_geoip.php");
	include_once($_SERVER['DOCUMENT_ROOT'] . "/core_config.php");
	include_once($_SERVER['DOCUMENT_ROOT'] . "/cp/cp_common.php");

	define( "CRAWL_TESTING_MODE", "0" ); //0 = normal submit, 1 = testing (not submitting to end users, but to us for debugging)
	define( "DEFLT_CRAWL_STEPS_PER_BATCH_TESTING", "1" );
	define( "DEFLT_CRAWL_STEPS_PER_BATCH_MANUAL", "3" );
	define( "DEFLT_CRAWL_STEPS_PER_BATCH_CRON", "10" );
	define( "DEFLT_SLEEP_BETWEEN_SUBMITS_SEC", "0" ); //in seconds, 0 to no sleep
	//define( "DEFAULT_SHOW_SITES_LIST", "1" );

	define( "DEFAULT_MAX_CONFIG_FILE_SIZE_KB", "2000" ); //2Mb. if the file will get bigger, let's stop crawling
	define( "DEFAULT_MAX_SITES_COUNT", "7000" ); //2Mb. if the file will get bigger, let's stop crawling
	define( "DEFAULT_MAX_CRAWL_DEPTH", "5" ); //how far we can go from entry point URL(s)
	define( "DEFAULT_MAX_REFS_COUNT", "3" );

	define( "MAX_CRON_TIME_MINUTES" , "10000" );

	define( "REF_URL_TEMPLATE", "https://www.vladonai.com/cms-bookmarks-panel?url=[url]&cat=-unsorted-&page=add-url-to-site" );

	//note: keep Entry names as short as possible and unique to allow fast array navigation and low memory footprint
	define( "ENTRY_SITES_LIST", 			"l" );
	define( "ENTRY_SITE_ID", 				"i" );
	define( "ENTRY_ENTRY_PAGE_URL", 		"u" );
	define( "ENTRY_NOTES", 					"n" );
	define( "ENTRY_B_CRAWL_WHOLE_DOMAIN",	"w" );
	define( "ENTRY_B_SITE_CRAWLED",			"c" );
	define( "ENTRY_STAUTS", 				"s" );
	define( "ENTRY_FETCH_TIME", 			"f" );
	define( "ENTRY_CRAWL_DATE",				"d" );
	define( "ENTRY_DEPTH", 					"e" );
	define( "ENTRY_REFS", 					"r" );
	define( "ENTRY_ALLOWED_URL_KEYWORDS", 	"k" );



	//error_reporting(E_ALL);
	//ini_set('display_errors', 1);

	global $b_cron_task;
	$b_cron_task = GetParam('cron') != "";

	ControlPanel_RenderPrintPageHeader("External sites crawling tool", "", !$b_cron_task);

	//
	global $b_test_mode; //enable it to debug
	$b_test_mode = CRAWL_TESTING_MODE == 0 ? false : true;
	//$b_test_mode = true;

	/* testing:
	if (!$b_cron_task)
	{
		.....
	}
	//*/

	$task_name = GetParam('task'); //ex: task_crawler_1, task_crawler_2

	if ($task_name == "")
	{
		if ($b_cron_task)
		{
			//echo "cron CP0 - entry<br>";
			HandleSingleCronSlotIfNeeded("task_crawler_1", CRON_SCRIPT_CRAWLER);
			HandleSingleCronSlotIfNeeded("task_crawler_2", CRON_SCRIPT_CRAWLER);
			HandleSingleCronSlotIfNeeded("task_crawler_3", CRON_SCRIPT_CRAWLER);
			HandleSingleCronSlotIfNeeded("task_crawler_4", CRON_SCRIPT_CRAWLER);
			HandleSingleCronSlotIfNeeded("task_crawler_5", CRON_SCRIPT_CRAWLER);
			HandleSingleCronSlotIfNeeded("task_crawler_6", CRON_SCRIPT_CRAWLER);
			HandleSingleCronSlotIfNeeded("task_crawler_7", CRON_SCRIPT_CRAWLER);
			HandleSingleCronSlotIfNeeded("task_crawler_8", CRON_SCRIPT_CRAWLER);
			HandleSingleCronSlotIfNeeded("task_crawler_9", CRON_SCRIPT_CRAWLER);
			HandleSingleCronSlotIfNeeded("task_crawler_10", CRON_SCRIPT_CRAWLER);
		} else
		{
			//list all available actions...
			$cron_config = LoadConfigStatus("cron_tasks");
			$b_save_config_changes = GetParam('submit') != ''; //ex: https://www.my_domain.com/cp/cp_mail_list_builder.php?cron_email_per_batch=4&manual_email_per_batch=2&show_usage_stats=show_usage_stats&task=task_minor_update_notify_3_31&save_changes=true

			if ($b_save_config_changes)
				$cron_config["b_enable_email_notifications"] = GetParam('b_enable_email_notifications') != "";

			echo   '<br><a href="' , GetThisUrl(false) , '.php">Refresh list!</a><br><br>';

			echo '<form method="post">';
			echo "<table cellpadding=3 style=\"margin:0px 0px 0 0px; border:1px solid #cccccc;\">
						<tr><td>Task:</td><td>Description:</td><td>Cron status</td><td>Message</td><td width=30%>Progress</td></tr>";

			function ShowCrawlingListTableRow($sub_task_name, $display_text, &$cron_config, &$rowN, $b_save_config_changes)
			{
				if ($b_save_config_changes)
				{
					$cron_config[$sub_task_name]['b_enable_cron'] = GetParam("b_enable_cron_" . $sub_task_name) != "";
					$cron_config[$sub_task_name]['cron_interval_minutes'] = GetParam("cron_interval_minutes_" . $sub_task_name);
					$cron_config[$sub_task_name]['task_description'] = GetParam("task_description_" . $sub_task_name);
				}
				$b_enable_cron_for_cur_task = $cron_config[$sub_task_name]['b_enable_cron'];
				
				$cron_interval_minutes = max(1, min(MAX_CRON_TIME_MINUTES, $cron_config[$sub_task_name]['cron_interval_minutes']));
				$cur_time_sec = time();
				
				$time_str_since_last_cron = round(($cur_time_sec - $cron_config[$sub_task_name]['cron_last_call_date_time']) / 60, 1);
				if ($time_str_since_last_cron < MAX_CRON_TIME_MINUTES)
				{
					$time_str_since_last_cron = conv_minutes_to_readable_time($time_str_since_last_cron);
				} else
				{
					$time_str_since_last_cron = "N/A min";
				}
				
				$b_show_progress = $cron_config[$sub_task_name]['b_enable_cron'] && $cron_config[$sub_task_name]['progress_percents'] < 100;
				$progress_percents = $b_show_progress ? $cron_config[$sub_task_name]['progress_percents'] : '';
				echo '<tr' . ($rowN % 2 == 1 ? ' bgcolor="#FFFEE7"' : '') . '>
						  <td><a href="' . GetThisUrl(false) . '.php?task=' . $sub_task_name . '">' . $display_text . '</a></td>
						  <td><input name="task_description_' . $sub_task_name . '" type="text" value="' . $cron_config[$sub_task_name]['task_description'] . '" /></td>
					      <td><input name="b_enable_cron_' . $sub_task_name . '" type="checkbox" value="b_enable_cron_' . $sub_task_name . '" ' . ($b_enable_cron_for_cur_task ? "checked" : "") . ' /> every 
					      	  <input name="cron_interval_minutes_' . $sub_task_name . '" type="text" value="' . $cron_interval_minutes . '" size="1"/> minutes</td>
						  <td> (last call ' . $time_str_since_last_cron . ' ago). ' . $cron_config[$sub_task_name]['last_msg_from_script'] . '</td>
						  <td>' , ($b_show_progress ? ControlPanel_RenderProgressBar($progress_percents, round($progress_percents, 2) . '%', false, false) : '') , '</td>
			      	  </tr>';
				$rowN ++;
			}

			ShowCrawlingListTableRow('task_crawler_1', 'Crawler #1', $cron_config, $rowN, $b_save_config_changes);
			ShowCrawlingListTableRow('task_crawler_2', 'Crawler #2', $cron_config, $rowN, $b_save_config_changes);
			ShowCrawlingListTableRow('task_crawler_3', 'Crawler #3', $cron_config, $rowN, $b_save_config_changes);
			ShowCrawlingListTableRow('task_crawler_4', 'Crawler #4', $cron_config, $rowN, $b_save_config_changes);
			ShowCrawlingListTableRow('task_crawler_5', 'Crawler #5', $cron_config, $rowN, $b_save_config_changes);
			ShowCrawlingListTableRow('task_crawler_6', 'Crawler #6', $cron_config, $rowN, $b_save_config_changes);
			ShowCrawlingListTableRow('task_crawler_7', 'Crawler #7', $cron_config, $rowN, $b_save_config_changes);
			ShowCrawlingListTableRow('task_crawler_8', 'Crawler #8', $cron_config, $rowN, $b_save_config_changes);
			ShowCrawlingListTableRow('task_crawler_9', 'Crawler #9', $cron_config, $rowN, $b_save_config_changes);
			ShowCrawlingListTableRow('task_crawler_10', 'Crawler #10', $cron_config, $rowN, $b_save_config_changes);

			echo "</table>";

			echo '<input name="b_enable_email_notifications" type="checkbox" value="b_enable_email_notifications"' . ($cron_config["b_enable_email_notifications"] ? "checked" : "") . ' />Enable <b>email notifications</b> on each crawl/ref batch.<br>';

			echo '<button type="submit" name="submit" value="submit">Save</button>';
			echo '</form>';

			if ($b_save_config_changes)
			{
				SaveConfigStatus("cron_tasks", $cron_config);
				echo "<br>Config data saved!...<br>";
			}

			//update page URL so it will not post same rate on page reload
			//echo "<script>document.location.href = \"test\";</script>";
		}
	} else
	{
		HandleSingleCrawlerBot($task_name);
	}

	function SortByRecordDate($record1, $record2)
	{
		$cmp = $record2["links_found"] - $record1["links_found"];

		if ($cmp == 0)
			$cmp = strcmp($record2[ENTRY_CRAWL_DATE], $record1[ENTRY_CRAWL_DATE]);

		if ($cmp == 0)
			$cmp = strcmp($record1[ENTRY_SITE_ID], $record2[ENTRY_SITE_ID]); //sort by name

		return $cmp;
	}

	function TmpBatchRenameSingleEntry($entry_old, $entry_new, &$encodedStr)
	{
		$old_str = 's:' . strlen($entry_old) . ':"' . $entry_old . '"';
		$new_str = 's:' . strlen($entry_new) . ':"' . $entry_new . '"';
		echo "old: " , $entry_old, "<br>new: " , $entry_new , "<br>";
		$encodedStr = str_replace($old_str, $new_str, $encodedStr); //to get rid from www.
	}

	function TmpBatchRenameKeysValues(&$config_file_data)
	{
		$encodedStr = serialize($config_file_data);
		echo "<br>old size: " , strlen($encodedStr) , "<br>" , substr($encodedStr, 0, 10000) , "<br><br> --------- <br><br>";

		TmpBatchRenameSingleEntry("list", ENTRY_SITES_LIST, $encodedStr);
		TmpBatchRenameSingleEntry("site_id", ENTRY_SITE_ID, $encodedStr);
		TmpBatchRenameSingleEntry("entry_page_url", ENTRY_ENTRY_PAGE_URL, $encodedStr);
		TmpBatchRenameSingleEntry("notes", ENTRY_NOTES, $encodedStr);
		TmpBatchRenameSingleEntry("b_crawl_whole_domain", ENTRY_B_CRAWL_WHOLE_DOMAIN, $encodedStr);
		TmpBatchRenameSingleEntry("b_site_crawled", ENTRY_B_SITE_CRAWLED, $encodedStr);
		TmpBatchRenameSingleEntry("status", ENTRY_STAUTS, $encodedStr);
		TmpBatchRenameSingleEntry("fetch_time", ENTRY_FETCH_TIME, $encodedStr);
		TmpBatchRenameSingleEntry("crawl_date", ENTRY_CRAWL_DATE, $encodedStr);
		TmpBatchRenameSingleEntry("depth", ENTRY_DEPTH, $encodedStr);
		TmpBatchRenameSingleEntry("refs", ENTRY_REFS, $encodedStr);

		echo "<br>new size: " , strlen($encodedStr) , "<br>" , substr($encodedStr, 0, 10000) , "<br>";

		echo "<br>list count (old): " , count($config_file_data["list"]);
		$config_file_data = unserialize($encodedStr);
		echo "<br>list count (new): " , count($config_file_data[ENTRY_SITES_LIST]);
	}

	function HandleSingleCrawlerBot($task_name)
	{
		global $task_name_minor_update_notify;
		global $b_test_mode; //enable it to debug
		global $b_cron_task;

		if ($task_name != "")
		{
			global $recent_app_ver_major;
			global $recent_app_ver_minor;  

			$submit_config_file_name = "config_crawler_" . $task_name . ".config_n";
		} else
		{
			echo "Error: Task name not specified or incorrect [{$task_name}].";

			return;
		}
		if (!$b_cron_task)
			echo "<br>Task <b>name</b>: {$task_name}<br>\r\n";

		$submit_config_file_content_txt = file_get_contents($submit_config_file_name);
		if ($submit_config_file_content_txt == "")
		{
			//reset to default state
			$config_file_data[ENTRY_SITES_LIST] = array();
			$config_file_data["log"] = array();
			$config_file_data["config"] = array();
			$config_file_data["config"]["cron_mode"] = "crawl";
			$config_file_data["config"]["b_show_site_check_stats"] = 1;
			$config_file_data["config"]["crawls_count_per_cron_batch"] = DEFLT_CRAWL_STEPS_PER_BATCH_CRON;
			$config_file_data["config"]["crawls_count_per_manual_batch"] = DEFLT_CRAWL_STEPS_PER_BATCH_MANUAL;
			$config_file_data["config"]["max_crawl_depth"] = DEFAULT_MAX_CRAWL_DEPTH;
			$config_file_data["config"]["max_refs_count"] = DEFAULT_MAX_REFS_COUNT;
			$config_file_data["config"]["max_file_size_kb"] = DEFAULT_MAX_CONFIG_FILE_SIZE_KB;
			$config_file_data["config"]["max_sites_count"] = DEFAULT_MAX_SITES_COUNT;
		} else
		{
			$config_file_data = unserialize($submit_config_file_content_txt);

			$b_clear_domains_with_errors = GetParam('b_clear_domains_with_errors') != "";
			$b_clear_domains_with_enough_refs = GetParam('b_clear_domains_with_enough_refs') != "";
			$b_clear_sites_ok_status = GetParam('b_clear_sites_ok_status') != "";
			$b_set_domains_with_errors_to_root = GetParam('b_set_domains_with_errors_to_root') != "";

			//
			$delN = 0;			
			$renN = 0;
			foreach ($config_file_data[ENTRY_SITES_LIST] as $key => &$cur_db_record)
			{
				$parsed_link_url = parse_url($cur_db_record[ENTRY_ENTRY_PAGE_URL]);
				$hostname = $parsed_link_url['host'];
				$hostname = str_replace("www.", "", $hostname);
				$hostname = strtolower($hostname);

				if ((stripos($cur_db_record[ENTRY_ENTRY_PAGE_URL], "worldcat.org/libraries") !== false && 
					 $cur_db_record[ENTRY_STAUTS] != ""
					 ) ||
					($b_clear_domains_with_errors && stripos($cur_db_record[ENTRY_STAUTS], "err") !== false) ||
					($b_clear_domains_with_enough_refs && $cur_db_record[ENTRY_REFS] >= $config_file_data["config"]["max_refs_count"]) ||
					stripos($cur_db_record[ENTRY_ENTRY_PAGE_URL], "goodreads.com") !== false ||
					stripos($cur_db_record[ENTRY_ENTRY_PAGE_URL], "sortis.ru") !== false ||
					stripos($cur_db_record[ENTRY_ENTRY_PAGE_URL], "rbc.ru") !== false ||
					stripos($cur_db_record[ENTRY_ENTRY_PAGE_URL], "debian.org") !== false ||
					stripos($cur_db_record[ENTRY_ENTRY_PAGE_URL], "habr.com") !== false ||
					stripos($cur_db_record[ENTRY_ENTRY_PAGE_URL], "polpred.com") !== false ||
					stripos($cur_db_record[ENTRY_ENTRY_PAGE_URL], "8500.ru") !== false)
				{
					$delN ++;
					if (!$b_cron_task)
						echo "unset ($delN): " , $cur_db_record[ENTRY_SITE_ID] , " - " , $key , "<br>";
					unset($config_file_data[ENTRY_SITES_LIST][$key]);
				} else
				if ($b_set_domains_with_errors_to_root && 
					stripos($cur_db_record[ENTRY_STAUTS], "err") !== false)
				{
					//leave only domain, cut off sub-path
					$err_page_url_parsed = parse_url($cur_db_record[ENTRY_ENTRY_PAGE_URL]);
					$corrected_url = ($err_page_url_parsed['scheme'] != "" ? ($err_page_url_parsed['scheme'] . '://') : "") . $err_page_url_parsed['host'] . "/";

					if ($corrected_url != $cur_db_record[ENTRY_ENTRY_PAGE_URL])
					{
						if (!$b_cron_task)
							echo "renaming ($renN): " . $cur_db_record[ENTRY_ENTRY_PAGE_URL] . " to " . $corrected_url . "<br>";
						$cur_db_record[ENTRY_ENTRY_PAGE_URL] = $corrected_url;
						$cur_db_record[ENTRY_STAUTS] = "";
						$renN ++;
					}
				} else
				if ($b_clear_sites_ok_status && 
					$cur_db_record[ENTRY_STAUTS] == "ok")
				{
					if (!$b_cron_task)
						echo "Clearing OK status for site " . $cur_db_record[ENTRY_SITE_ID] . "<br>";
					$cur_db_record[ENTRY_STAUTS] = "";
				}
			}
		}

		$b_save_config_changes = GetParam('save_changes') == 'true';
		if ($b_save_config_changes)
		{
			$config_file_data["config"]["cron_mode"] = GetParam('cron_mode');
			$config_file_data["config"]["b_show_site_check_stats"] = GetParam('show_site_check_stats') != "";
			$config_file_data["config"]["crawls_count_per_cron_batch"] = GetConfigParamValue("crawls_count_per_cron_batch", DEFLT_CRAWL_STEPS_PER_BATCH_CRON, 1, 50, 1);
			$config_file_data["config"]["crawls_count_per_manual_batch"] = GetConfigParamValue("crawls_count_per_manual_batch", DEFLT_CRAWL_STEPS_PER_BATCH_MANUAL, 1, 50, 1);
			$config_file_data["config"]["max_crawl_depth"] = GetConfigParamValue("max_crawl_depth", DEFAULT_MAX_CRAWL_DEPTH, 1, 50);
			$config_file_data["config"]["max_refs_count"] = GetConfigParamValue("max_refs_count", DEFAULT_MAX_REFS_COUNT, 1, 1000);
			$config_file_data["config"]["max_file_size_kb"] = GetConfigParamValue('max_file_size_kb', DEFAULT_MAX_CONFIG_FILE_SIZE_KB, 1, 30000);
			$config_file_data["config"]["max_sites_count"] = GetConfigParamValue("max_sites_count", DEFAULT_MAX_SITES_COUNT, 1, 30000);
			$config_file_data["config"]["b_show_all_sites_list"] = GetParam('b_show_all_sites_list');
		}

		$b_show_all_sites_list = $config_file_data["config"]["b_show_all_sites_list"];
		$max_config_file_size_reached = $config_file_size > ($config_file_data["config"]["max_file_size_kb"] * 1024);
		$max_list_size_reached = count($config_file_data[ENTRY_SITES_LIST]) > $config_file_data["config"]["max_sites_count"];

		$cron_interval_minutes = max(1, min(MAX_CRON_TIME_MINUTES, $cron_config[$sub_task_name]['cron_interval_minutes']));

		//check for another instance of batch is running simultaneously
		if ($config_file_data["config"]["b_processing_in_progress"])
		{
			$allowed_execution_time = /*$config_file_data["config"]["sleep_time_sec_between_submits"] **/ max($config_file_data["config"]["crawls_count_per_cron_batch"], $config_file_data["config"]["crawls_count_per_manual_batch"]) * 3;
			if ((time() - $config_file_data["config"]["b_processing_in_progress_start_time"]) < $allowed_execution_time)
			{
				if (!$b_cron_task)
					echo "ANOTHER INSTANCE BATCH IS RUNNING!!! EXIT! Reload this page bit later, in about " . abs((time() - $config_file_data["config"]["b_processing_in_progress_start_time"]) - $allowed_execution_time) . " seconds.";
				exit; //another batch sending in progress, exiit immediately!
			}
		}

		$config_file_data["config"]["b_processing_in_progress"] = true;
		$config_file_data["config"]["b_processing_in_progress_start_time"] = time(); //time in seconds since the Unix Epoch
		//save b_processing_in_progress state asap
		$submit_config_file_content_txt = serialize($config_file_data);
		if (!file_put_contents($submit_config_file_name, $submit_config_file_content_txt, LOCK_EX))
			if (!$b_cron_task)
				echo "<font color=red>ERROR! Cannot save config file {$submit_config_file_name}!!!</font><br>";

		//
		$successful_sites_crawled = 0;
		$failed_sites = 0;
		$start_time = microtime_float();
		$cur_date_time = strtotime('now');

		$b_start_single_site_crawling = GetParam('subtask') == 'crawl_site_now';
		$b_delete_single_site = GetParam('subtask') ==  'delete_site';
		$single_site_to_handle_id = urldecode(GetParam('site_id'));

		$b_start_new_batch_portion_crawl = $b_cron_task ? $config_file_data["config"]["cron_mode"] == "crawl" : stripos(GetParam('start'), "crawl") !== false;
		$b_start_new_batch_portion_ref = $b_cron_task ? $config_file_data["config"]["cron_mode"] == "ref" : stripos(GetParam('start'), "ref") !== false;
		$b_start_new_batch_portion = $b_start_new_batch_portion_crawl || $b_start_new_batch_portion_ref;

		if ($b_cron_task && !$b_start_new_batch_portion)
			exit; //nothing to do

		$portion_amount_sites = $b_test_mode ? CRAWL_STEPS_PER_BATCH_TESTING : ($b_cron_task ? $config_file_data["config"]["crawls_count_per_cron_batch"] : $config_file_data["config"]["crawls_count_per_manual_batch"]);
		$site_n_in_cur_portion = 0; //used when $b_start_new_batch_portion == ture
		$this_url = GetThisUrl(false);

		$output_buffer = "";

		if (!$b_cron_task)
		{
			echo '<table width="100%" cellpadding="3" style="margin:0px 0px 0 0px; border:1px solid #cccccc; font-family: Verdana; font-size: 12px" >';
			echo '<tr bgcolor="#FFFFFF"><td>';

			//
			echo '<td>';
			echo ' <b>Navigation:</b><br>';
			echo ' <a href="' . $this_url . '.php">Crawling <b>Home</b>...</a><br>';
			echo ' <a href="' . $this_url . '.php?task=' . $task_name . '&start=crawl"><b>Start</b> single batch of manual Crawling...</a><br>';
			echo ' <a href="' . $this_url . '.php?task=' . $task_name . '&start=ref"><b>Start</b> single batch of manual Ref Poking...</a><br>';
			echo ' <a href="' . $this_url . '.php?task=' . $task_name . '&b_set_domains_with_errors_to_root=1">Set domains with <b>error</b> to <b>root</b> page.</a><br>';
			echo ' <a href="' . $this_url . '.php?task=' . $task_name . '&b_clear_domains_with_errors=1">Remove domains with <b>error</b> from the list.</a><br>';
			echo ' <a href="' . $this_url . '.php?task=' . $task_name . '&b_clear_domains_with_enough_refs=1">Remove domains with enough <b>refs</b> from the list.</a><br>';
			echo ' <a href="' . $this_url . '.php?task=' . $task_name . '&b_clear_sites_ok_status=1">Clear site <b>OK status</b> (to start crawling after increasing depth).</a><br>';
			echo ' <a href="' . $this_url . '.php?task=' . $task_name . '&b_reload_db=1">Reload sites from DB.</a><br>';
			echo ' <a href="' . $this_url . '.php?task=' . $task_name . '">Reload stats for this list (<b>refresh</b>).</a><br>';
			echo '</td>';

			//Generate mailer settings UI code
			echo '<td>';
			echo ' <form method="post">';
			echo ' <b>Current PAD submit list settings:</b><br>';
			echo '	Cron mode: 
						<select id="cron_mode" name="cron_mode"">
				  			<option value="crawl"' . ($config_file_data["config"]["cron_mode"] == "crawl" ? " selected" : "") . '>Crawl</option>
							<option value="ref"' . ($config_file_data["config"]["cron_mode"] == "ref" ? " selected" : "") . '>Ref</option>
						</select><br>';
			echo '	Submits # per cron batch: <input name="crawls_count_per_cron_batch" type="text" value="' . $config_file_data["config"]["crawls_count_per_cron_batch"] . '" /> <br />';
			echo '	Submits # per manual batch: <input name="crawls_count_per_manual_batch" type="text" value="' . $config_file_data["config"]["crawls_count_per_manual_batch"] . '" /> <br />';
			echo '	Max crawl depth: <input name="max_crawl_depth" type="text" value="' . $config_file_data["config"]["max_crawl_depth"] . '" /> <br />';
			echo '	Max refs count per site: <input name="max_refs_count" type="text" value="' . $config_file_data["config"]["max_refs_count"] . '" /> <br />';
			$config_file_size = filesize($submit_config_file_name);
			$config_file_size_percents = sprintf("%.2f", ($config_file_size / ($config_file_data["config"]["max_file_size_kb"] * 1024)) * 100);
			echo '	Max config file size (KBytes): <input name="max_file_size_kb" type="text" value="' . $config_file_data["config"]["max_file_size_kb"] . '" /> [' . round($config_file_size / 1024) . 'Kb]=[' . $config_file_size_percents . '%]<br />';
			$sites_fullfulled = count($config_file_data[ENTRY_SITES_LIST]);
			$sites_fullfulled_percents = sprintf("%.2f", ($sites_fullfulled / $config_file_data["config"]["max_sites_count"]) * 100);
			echo '	Max sites count in DB: <input name="max_sites_count" type="text" value="' . $config_file_data["config"]["max_sites_count"] . '" /> [' . $sites_fullfulled . ']=[' . $sites_fullfulled_percents . '%]<br />';
			echo '	<input name="b_show_all_sites_list" type="checkbox" value="b_show_all_sites_list" ' , ($b_show_all_sites_list ? "checked" : "") , ' />Show all sites list<br />';
			echo '	<input type="hidden" name="task" value="' . $task_name . '"><br />';
			echo '	<input type="hidden" name="save_changes" value="true"><br />';
			echo '	<button type="submit" value="Submit">Save</button>';
			echo ' </form>';
			echo '</td>';

			//Log
			echo '<td>';
			echo ' <b>Log of last submits activity (max 10)</b><br>';
			$log_size = count($config_file_data["log"]);
			if ($log_size > 0)
			{
				for ($lastLogN = $log_size - 1; $lastLogN >= 0; $lastLogN --)
				{
					if ($lastLogN >= max(0, $log_size - 10))
						echo $lastLogN . ": " . $config_file_data["log"][$lastLogN] . "<br>";
					else
						unset($config_file_data["log"][$lastLogN]);
				}

				$config_file_data["log"] = array_values($config_file_data["log"]);
			} else
			{
				echo '- empty -';
			}
			echo '</td>';

			echo '</tr></table>';

			//make table sortable
			echo '<script type="text/javascript"> (function($) {
						var ajaxurl = "https://www.bu.edu/tech/wp-admin/admin-ajax.php";
						if (jQuery(\'.sortable\').length) {
							var data = {
								action: \'sortable_scripts\'
							};
						$.post(ajaxurl, data, function(scripts) {
							var s = document.getElementsByTagName(\'script\')[0];
							$(s).before(scripts);
						});
					}
				})(jQuery); </script>';

			//Submits table header
			if ($b_show_all_sites_list)
			{
				$output_buffer .= '<table id="bigTable" cellpadding="3" style="margin:0px 0px 0 0px; border:1px solid #cccccc; font-family: Verdana; font-size: 12px; display: none;"  class="sortable">';
				$header_html_text = '<thead><tr bgcolor="#FBBF99" style="font-weight: bold"> 
					<th>^ </th> 
					<th>Rank</th> 
					<th>When</th> 
					<th>Refs</th> 
					<th>Depth</th> 
					<th>Speed</th> 
					<th>Domain</th> 
					<th>Links</th> 
					<th>Actions</th> 
					<th>Notes</th> 
					<th>Status</th> 
					' . "</tr></thead> \r<tbody> \r";
				$output_buffer .= $header_html_text;
			}
		}

		$b_show_site_check_stats = $config_file_data["config"]["b_show_site_check_stats"];
		$b_reload_data_from_db = count($config_file_data[ENTRY_SITES_LIST]) == 0 || GetParam('b_reload_db') != "";

		if ($b_clear_domains_with_errors)
			echo "Searching for domains with <b>error</b> code ('err')...<br>";

		if ($b_clear_domains_with_enough_refs)
			echo "Searching for domains with <b>enough refs</b> to delete them from the list...<br>";

		if ($b_clear_sites_ok_status)
			echo "Clearing site OK status...<br>";

		if ($b_set_domains_with_errors_to_root)
			echo "Reverting domains with <b>error</b> code ('err') to <b>root page</b> of that domain...<br>";

		if (count($config_file_data[ENTRY_SITES_LIST]) == 0)
			$config_file_data[ENTRY_SITES_LIST] = array();

		$current_microtime = microtime_float();
		$rendering_time_seconds = sprintf("%.4f", $current_microtime - $start_time);

		$records_found = 0;
		$sites_with_site_ref_missing = 0;
		$sites_with_site_crawl_missing = 0;
		$sites_with_error_404_count = 0;
		$sites_with_error_timeout_count = 0;
		$sites_with_error = 0;

		if ($b_reload_data_from_db)
		{
			/////////////////////////////////////////////////////
			//	Read PAD repositories/sites from config file

			$hardcoded_sites_list = get_hardcoded_site_repositories_list($task_name);
			echo "<span style=\"color:#44cc00;font-size: 9px;\">Reading from DB, " . count($hardcoded_sites_list) . " records found</span><br>";

			foreach ($hardcoded_sites_list as $key => $cur_db_record)
			{
				$site_id = $cur_db_record[ENTRY_SITE_ID];

				if (!array_key_exists($site_id, $config_file_data[ENTRY_SITES_LIST]))
					$config_file_data[ENTRY_SITES_LIST][$site_id] = array();

				$config_file_data[ENTRY_SITES_LIST][$site_id] = array_merge($config_file_data[ENTRY_SITES_LIST][$site_id], $cur_db_record);
			}
		}

		//////////////////////////////////////////////////////////////////////////////////
		//
		//	Sort array by date (as it may be merged of old and updated from DB data)
		//

		uasort($config_file_data[ENTRY_SITES_LIST], 'SortByRecordDate');

		//
		if ($b_delete_single_site && $single_site_to_handle_id != "")
		{
			if (!$b_cron_task)
				echo "Deteling site: " . $single_site_to_handle_id . "<br>";
			unset($config_file_data[ENTRY_SITES_LIST][$single_site_to_handle_id]);
		}

		/////////////////////////////////////////////////////
		//
		//	parse records/display/crawl sites (if needed)
		//

		$cumulative_list_of_all_sites = "";
		$cumulative_list_of_all_sites_crawled = "";
		$cumulative_list_of_new_sites_added = "";
		$estimated_refs_count_to_complete = 0;
		$b_crawling_list_was_modified = false;

		$threadsCount = 0;
		$workerThreads = [];

		if ($b_start_single_site_crawling && $single_site_to_handle_id != '')
		{
			//Crawl!

			$b_success = CrawlNewUrl($config_file_data, $single_site_to_handle_id, $task_name, $extra_params, $cumulative_list_of_new_sites_added);
			if ($b_success)
			{
				$successful_sites_crawled ++;
				$b_crawling_list_was_modified = true;
			} else
			{
				$failed_sites ++;
				$cumulative_list_of_all_sites_crawled .= " ------";
			}
			$cumulative_list_of_all_sites_crawled .= "[" . /*$site_id*/ $config_file_data[ENTRY_SITES_LIST][$site_id][ENTRY_ENTRY_PAGE_URL] . "] ";
		}

		foreach ($config_file_data[ENTRY_SITES_LIST] as $key => &$cur_db_record)
		{
			$site_id = $cur_db_record[ENTRY_SITE_ID];

			$crawl_date_time = $cur_db_record[ENTRY_CRAWL_DATE];
			$crawl_date_str = $crawl_date_time != 0 ? date("M j, H:i", $crawl_date_time) : "";
			$crawl_depth = $cur_db_record[ENTRY_DEPTH];
			if ($crawl_depth != "")
				$crawl_depth_str = '' . (int)$crawl_depth;
			else
				$crawl_depth_str = "";

			$refs_count = $cur_db_record[ENTRY_REFS] == "" ? 0 : (int)$cur_db_record[ENTRY_REFS];
			$refs_count_str = $refs_count;
			if ($refs_count_str > 0)
				$refs_count_str = '' . (int)$refs_count_str;
			else
				$refs_count_str = "";

			$links_to_this_site_found_count = $cur_db_record["links_found"] == "" ? 0 : (int)$cur_db_record["links_found"];
			$links_to_this_site_found_count_str = $links_to_this_site_found_count;
			if ($links_to_this_site_found_count_str > 0)
			{
				$links_to_this_site_found_count_str = '' . (int)$links_to_this_site_found_count_str;
			} else
			{
				$links_to_this_site_found_count_str = "";
			}
			
			$bg_color_str = ($records_found % 2 == 1 ? " bgcolor=\"#FFFEE7\"" : "");

			$site_url = $cur_db_record[ENTRY_ENTRY_PAGE_URL];
			$b_crawl_whole_domain = $cur_db_record[ENTRY_B_CRAWL_WHOLE_DOMAIN];

			$b_error_404 = stripos($cur_db_record[ENTRY_STAUTS], "err - 404") !== false;
			$b_error_timedout = stripos($cur_db_record[ENTRY_STAUTS], "err - timeout") !== false;
			$b_error_generic = $b_error_timedout || $b_error_404 || stripos($cur_db_record[ENTRY_STAUTS], "err") !== false;
			if ($b_error_404)
				$sites_with_error_404_count ++;
			if ($b_error_timedout)
				$sites_with_error_timeout_count ++;
			if ($b_error_generic)
				$sites_with_error ++;

			$b_blacklisted = stripos($cur_db_record[ENTRY_STAUTS], "skip") !== false || $b_error_generic;
			$b_crawled_already = stripos($cur_db_record["submits"], $task_name) !== false || $cur_db_record[ENTRY_B_SITE_CRAWLED] == true;
			$cur_db_record[ENTRY_B_SITE_CRAWLED] = $b_crawled_already;

			$b_no_autosubmit = $cur_db_record["b_no_autosubmit"] == "1";

			$b_site_alread_present_above = stripos($cumulative_list_of_all_sites, "[" . $site_id . "]") !== FALSE;
			$cumulative_list_of_all_sites .= "[" . $site_id . "]";

			$b_ok_to_crawl_site = $crawl_depth <= $config_file_data["config"]["max_crawl_depth"];
			$b_ok_to_ref_site = $refs_count < $config_file_data["config"]["max_refs_count"];
			$skip_reason_str = "";
			$b_ignore_site = $b_blacklisted || 
							 $b_site_alread_present_above || 
							 (!$b_ok_to_crawl_site && $b_start_new_batch_portion_crawl) || 
							 (!$b_ok_to_ref_site && $b_start_new_batch_portion_ref) ||
							 !is_url_ok_to_be_crawled($site_url, "", $skip_reason_str);

			$b_cur_site_is_being_processed = false;
			$strikethrough_tag = $b_ignore_site ? "<s>" : "</s>";

			$record_site_check_stats_array = unserialize(stripslashes($cur_db_record["site_check_stats"]));

			$last_listed_app_ver = (is_countable($record_site_check_stats_array) && count($record_site_check_stats_array) > 1) ? $record_site_check_stats_array['last_seen_app_ver'] : 0;

			if ($b_ok_to_crawl_site && !$b_ignore_site && !$b_crawled_already)
				$sites_with_site_crawl_missing ++;
			if ($b_ok_to_ref_site && !$b_ignore_site)
			{
				$sites_with_site_ref_missing ++;
				$estimated_refs_count_to_complete += max(0, $config_file_data["config"]["max_refs_count"] - $refs_count);
			}

			if ($b_ignore_site)
			{
				if (!$b_cron_task)
					echo 'Ignored: ' . $hostname . '<br>&nbsp;&nbsp;&nbsp;&nbsp;' . $link_visible_text . ' === ' . $link_domain . ' +++ ' . $link_url . ', reason: <b>' . $skip_reason_str . '</b><br>';
			} else
			if ((!$b_crawled_already || ($b_start_new_batch_portion_ref && !$b_start_new_batch_portion_crawl)) &&
				!$b_no_autosubmit)
			{
				//that's our client! :)
				$cur_db_record[ENTRY_B_SITE_CRAWLED] = $b_crawled_already || $cur_db_record[ENTRY_B_SITE_CRAWLED];

				if ($b_start_new_batch_portion &&
					$portion_amount_sites > $site_n_in_cur_portion &&
					($cur_db_record[ENTRY_B_SITE_CRAWLED] == false || !$b_crawled_already || ($b_start_new_batch_portion_ref && !$b_start_new_batch_portion_crawl)))
				{
					$b_cur_site_is_being_processed = true;
					$site_n_in_cur_portion ++;

					//echo "...sent emulation...";

					if ($max_config_file_size_reached && !$b_start_new_batch_portion_ref)
					{
						if (!$b_cron_task)
							echo "Error: Max config file size reached, cannot crawl! Incerase MAX_CONFIG_FILE_SIZE_KB if you want more crawls";
					} else
					if ($max_list_size_reached && !$b_start_new_batch_portion_ref)
					{
						if (!$b_cron_task)
							echo "Error: Max sites limit reached, cannot crawl! Incerase MAX_SITES_COUNT if you want more crawls";
					} else
					{
						//Crawl!
						$b_ref_only = !$b_start_new_batch_portion_crawl && $b_start_new_batch_portion_ref;

						$b_success = CrawlNewUrl($config_file_data, $site_id, $task_name, $extra_params, $cumulative_list_of_new_sites_added, $b_ref_only);
						if ($b_success)
						{
							$successful_sites_crawled ++;
							$b_crawling_list_was_modified = true;
						} else
						{
							$failed_sites ++;
							$cumulative_list_of_all_sites_crawled .= " ------";
						}
						$fetch_time_str = $cur_db_record[ENTRY_FETCH_TIME] != "" ? (" +" . $cur_db_record[ENTRY_FETCH_TIME] . "sec") : "";
						$cumulative_list_of_all_sites_crawled .= "[" . /*$site_id*/ $config_file_data[ENTRY_SITES_LIST][$site_id][ENTRY_ENTRY_PAGE_URL] . ($b_start_new_batch_portion_ref ? (' rf:' . $refs_count_str) : "") . $fetch_time_str . "] ";
					}
				}
			}

			if ($b_show_site_check_stats)
			{
				if (is_countable($record_site_check_stats_array) &&
				 	count($record_site_check_stats_array) > 1)
				{
					$checked_days_ago = round(($cur_date_time - strtotime($record_site_check_stats_array['checked_days_ago'])) / (60 * 60 * 24));
					$versions_str = $record_site_check_stats_array['last_seen_app_ver'];
					$site_check_stats_html = " <td>" . ($b_ignore_site ? "" : "<b>") . "{$checked_days_ago} days" . ($b_ignore_site ? "" : "</b>") . "</td> 
						<td>{$versions_str}</td>  ";
				} else
				{
					$site_check_stats_html = "";
				}
			}

			$record_status_str = $records_found . ($b_cur_site_is_being_processed ? " <b>Prcssng...<b>" : "") . ($cur_db_record[ENTRY_B_SITE_CRAWLED] == true || $b_crawled_already ? " cr:)" : "");

			$crawl_site_now_url = GetThisUrl(false) . '.php?task=' . $task_name . '&subtask=crawl_site_now&site_id=' . urlencode($site_id);
			$delete_site_now_url = GetThisUrl(false) . '.php?task=' . $task_name . '&subtask=delete_site&site_id=' . $key;

			$notes = $cur_db_record[ENTRY_NOTES];

			$satus = $cur_db_record[ENTRY_STAUTS];

			$fetch_time_str = $cur_db_record[ENTRY_FETCH_TIME] != "" ? (" " . $cur_db_record[ENTRY_FETCH_TIME] . "sec") : "";
			if ($cur_db_record[ENTRY_FETCH_TIME] > 3)
				$fetch_time_str = "<b>" . $fetch_time_str . "</b>"; //bold text

			$cur_output_line = "<tr {$bg_color_str}>
				<td>" . $record_status_str . "</td>
				<td>{$strikethrough_tag}" . ($b_ignore_site ? "---" : "") . "</s></td>
				<td>{$strikethrough_tag}{$crawl_date_str}</s></td>
				<td>{$strikethrough_tag}{$refs_count_str}</s></td>
				<td>{$strikethrough_tag}{$crawl_depth_str}</s></td>
				<td>{$strikethrough_tag}{$fetch_time_str}</s></td>
				<td>{$strikethrough_tag}<a href=\"{$site_url}\"  target=\"_blank\">{$site_id}</a>" . ($b_crawl_whole_domain ? " <span title=\"Crawl all pages on this domain\">ALL</span>" : "") . "</s></td>
				<td>{$strikethrough_tag}{$links_to_this_site_found_count_str}</s></td>
				<td>{$strikethrough_tag}<a href=\"{$crawl_site_now_url}\">Crawl now!</a> -- <a href=\"{$delete_site_now_url}\">Del</a></s></td>
				<td>{$strikethrough_tag}" . ($notes != "" ? '<span title="' . $notes . '">note</span>' : "") . "</s></td>
				<td>{$strikethrough_tag}{$satus}</s></td>
			   </tr> \r";
			$output_buffer .= $cur_output_line;

			if ($records_found % 31 == 30)
				$output_buffer .= $header_html_text;
			
			$records_found ++;
		}

		///////////////////////////////////
		//	wait for threads to finish
		foreach($workerThreads as $curThread)
		{
		    $curThread->join();
			$b_success = $curThread->m_bSuccess;
			if ($b_success)
			{
				$successful_sites_crawled ++;
				$b_crawling_list_was_modified = true;
			} else
			{
				$failed_sites ++;
				$cumulative_list_of_all_sites_crawled .= " ------";
			}
			$cumulative_list_of_all_sites_crawled .= "[" . $curThread->m_single_site_to_handle_id . "] ";
		}

		/////////////////////////////////
		//	do screen output

		//////////////////////////////////////////
		//
		//	done parsing records
		//
		$percents_done_crawl = (10000 - round(($sites_with_site_crawl_missing / $records_found) * 10000)) / 100;
		$percents_done_ref = (10000 - round(($sites_with_site_ref_missing / $records_found) * 10000)) / 100;

		$eta_on_all_crawl_sites_in_minutes = ($records_found * $cron_interval_minutes) / $config_file_data["config"]["crawls_count_per_cron_batch"];
		$eta_on_remaining_crawl = conv_minutes_to_readable_time(((100 - $percents_done_crawl) * $eta_on_all_crawl_sites_in_minutes) / 100);
		$eta_remaining_on_all_ref_sites_in_minutes = ($estimated_refs_count_to_complete * $cron_interval_minutes) / $config_file_data["config"]["crawls_count_per_cron_batch"];
		$eta_on_remaining_ref = conv_minutes_to_readable_time($eta_remaining_on_all_ref_sites_in_minutes);

		if (!$b_cron_task)
		{
			///////////////////////////////////////
			//
			//	Show mailing status summary
			//

			if ($records_found > 0)
				echo "<font size=\"+3\">Progress (ref): <b>{$percents_done_ref}% done</b>" . ($percents_done_ref < 100 ? ", ETA $eta_on_remaining_ref" : "") . "</font>" ,
					  ControlPanel_RenderProgressBar($percents_done_ref, round($percents_done_ref, 2) . '%', false, false) ,
					  "<br><font size=\"+3\">Progress (crawl): <b>{$percents_done_crawl}% done</b>" . ($percents_done_crawl < 100 ? ", ETA $eta_on_remaining_crawl" : "") . "</font>" ,
					  ControlPanel_RenderProgressBar($percents_done_crawl, round($percents_done_crawl, 2) . '%', false, false) ,
					  "<br>
					  Sites count: <b>$records_found</b><br />
					  Sites with errors: $sites_with_error ($sites_with_error_404_count (not found) + $sites_with_error_timeout_count (timeout))<br />";
			else
				echo "NO RECORDS FOUND!<br>";

			if ($b_start_new_batch_portion)
			{
				echo "Starting new crawling batch portion (<b>" . $portion_amount_sites . "</b> pack)...<br>";
				echo "Crawling batch complete.<br>";
			}

			if ($b_show_all_sites_list)
			{
				echo $output_buffer;
				echo '</tbody></table>';
			}
			$output_buffer = "";
		}

		//log
		if ($successful_sites_crawled > 0 || $failed_sites > 0)
		{
			$log_entry_key = count($config_file_data["log"]);
			$config_file_data["log"][$log_entry_key] = date("G:i M j - ") . ($b_cron_task ? "cron" : "manual") . ", " . ($b_start_new_batch_portion_ref ? $percents_done_ref : $percents_done_crawl) .  "%, success $successful_sites_crawled, fails $failed_sites";
		}

		if ($successful_sites_crawled > 0 || $failed_sites > 0)
		{
			$cron_config = LoadConfigStatus("cron_tasks");

			if ($cron_config["b_enable_email_notifications"])
			{
				//email cron result report
				$msg = "Task: {$task_name} - https://www.my_domain.com/cp/cp_ext_sites_crawler.php?task={$task_name}\r\n\r\nSuccessfull " . ($b_start_new_batch_portion_ref ? 'refs' : 'crawls') . 
					   ": {$successful_sites_crawled}\r\nNot sent due to errors: {$failed_sites}\r\n\r\n" . 
					   "Sites: {$cumulative_list_of_all_sites_crawled}\r\n" .
					   "Added: {$cumulative_list_of_new_sites_added}\r\n" .
					   "<font " . ($b_start_new_batch_portion_ref ? "size=\"+3\"" : "") . ">Progress (ref): <b>{$percents_done_ref}% done</b></font><br>\r\n" . 
					   "<font " . ($b_start_new_batch_portion_crawl ? "size=\"+3\"" : "") . ">Progress (crawl): <b>{$percents_done_crawl}% done</b></font><br>\r\n" .
					   " remains, out of " . $records_found . "\r\n";

				simple_email('sys@my_domain.com', ($b_start_new_batch_portion_ref ? 'Ref' : 'Crawler') . ' bot cron task ' . $task_name . ' report (' . $successful_sites_crawled . '/' . ($successful_sites_crawled + $failed_sites) . ' success rate)', $msg, 'sys@my_domain.com'); 
				if (!$b_cron_task)
					echo $msg;
			}
		}

		$b_turn_off_cron = false;
		if ($b_cron_task &&
			(!$b_start_new_batch_portion_ref || $percents_done_ref >= 100) &&
			(!$b_start_new_batch_portion_crawl || $percents_done_crawl >= 100))
		{
			//Cron task is Finished. Turn it off to reduce CPU/disk loading.
			$b_turn_off_cron = true;

			//email us to notify about cron task complete
			$msg = "Task: {$task_name} - https://www.my_domain.com/cp/cp_ext_sites_crawler.php?task={$task_name}\r\n\r\n
			FINISHED! Cron task finished and got turned off. Enjoy :)\r\n";

			simple_email('sys@my_domain.com', ($b_start_new_batch_portion_ref ? 'Ref' : 'Crawler') . ' bot cron task ' . $task_name . ' FINISHED', $msg, 'sys@my_domain.com'); 
			if (!$b_cron_task)
				echo $msg;
		}

		///////////////////////////////////////
		//
		//	Save config file...
		//

		//save anyway as we are modifying b_processing_in_progress... old: if ($b_reload_data_from_db || $b_crawling_list_was_modified || $b_save_config_changes)
		if (!$b_cron_task)
			echo "<br>Saving data...";
		$config_file_data["config"]["b_processing_in_progress"] = false;
		$submit_config_file_content_txt = serialize($config_file_data);
		if (file_put_contents($submit_config_file_name, $submit_config_file_content_txt, LOCK_EX))
			echo !$b_cron_task ? " Saved! <br>" : "";
		else
			echo "<font color=red>ERROR! Cannot save config file {$submit_config_file_name}!!!</font><br>";

		if ($b_cron_task)
		{
			$cron_config = LoadConfigStatus("cron_tasks");

			$eta_on_remaining_cron_in_minutes = ($b_start_new_batch_portion_ref ? ($eta_remaining_on_all_ref_sites_in_minutes) : ($percents_done_crawl * $eta_on_all_crawl_sites_in_minutes));

			$cron_msg = date("G:i M j - ") . 
						($b_start_new_batch_portion_ref ? ("ref <b>" . $percents_done_ref) : ("crawl <b>" . $percents_done_crawl)) .  "%</b>" .
						", ETA " . ($b_start_new_batch_portion_ref ? $eta_on_remaining_ref : $eta_on_remaining_crawl) . 
						", success $successful_sites_crawled" . 
						($failed_sites > 0 ? ", fails $failed_sites" : "") . 
						($b_start_new_batch_portion_crawl ? ", size: $records_found" : "");

			if ($b_turn_off_cron)
			{
				$cron_config[$task_name]['b_enable_cron'] = false;
				$cron_msg = "<b>FINISHED!</b> " . $cron_msg;
			}
			$cron_config[$task_name]['last_msg_from_script'] = $cron_msg;
			$cron_config[$task_name]['progress_percents'] = $b_start_new_batch_portion_ref ? $percents_done_ref : $percents_done_crawl;
			SaveConfigStatus("cron_tasks", $cron_config);
		}

		if (!$b_cron_task)
			echo "Done :)<br>";
	}

	function file_get_contents_for_crawling($url, $referer = "", $b_include_header = FALSE, $b_async_exc_allowed = false)
	{
		$url = trim($url);

		{
			$header_list = array('Accept-Charset: ISO-8859-1,utf-8;q=0.7,*;q=0.7',
								 'Accept: application/xml,application/xhtml+xml,text/html;q=0.9,text/plain;q=0.8,image/png,*/*;q=0.5',
								 'Accept-Language: en-US,en;q=0.8');

			$ualist = array(
								'Mozilla/5.0 (Linux; Android 8.0.0; SM-G960F Build/R16NW) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/62.0.3202.84 Mobile Safari/537.36', //samsung galaxy s9
								'Mozilla/5.0 (Linux; Android 4.4.3; KFTHWI Build/KTU84M) AppleWebKit/537.36 (KHTML, like Gecko) Silk/47.1.79 like Chrome/47.0.2526.80 Safari/537.36', //Amazon Kindle Fire HDX 7
								'Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/78.0.3904.108 Safari/537.36', //Windows 10-based PC using Edge browser
								'Mozilla/5.0 (X11; CrOS x86_64 8172.45.0) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/51.0.2704.64 Safari/537.36', //Chrome OS-based laptop using Chrome browser (Chromebook)
								'Mozilla/5.0 (X11; Ubuntu; Linux x86_64; rv:15.0) Gecko/20100101 Firefox/15.0.1', //Linux-based PC using a Firefox browser
								'AppleTV6,2/11.1', //Apple TV 5th Gen 4K
								'Mozilla/5.0 (X11; U; Linux armv7l like Android; en-us) AppleWebKit/531.2+ (KHTML, like Gecko) Version/5.0 Safari/533.2+ Kindle/3.0+' //Amazon kindle 4
								);
			$uagent = $ualist[rand(0, count($ualist) - 1)];
			$ch = curl_init();
			curl_setopt($ch, CURLOPT_HEADER, $b_include_header); //include the header in the output.
			curl_setopt($ch, CURLINFO_HEADER_OUT, 1);
			curl_setopt($ch, CURLOPT_RETURNTRANSFER, TRUE); //return the transfer as a string, instead of printing it to the browser.
			if ($referer != "")
				curl_setopt($ch, CURLOPT_REFERER, $referer);

			curl_setopt($ch, CURLOPT_URL, $url);
			curl_setopt($ch, CURLOPT_USERAGENT, $uagent);
			curl_setopt($ch, CURLOPT_HTTPHEADER, $header_list);
			curl_setopt($ch, CURLOPT_FOLLOWLOCATION, TRUE);
			curl_setopt($ch, CURLOPT_MAXREDIRS, 8);
			curl_setopt($ch, CURLOPT_CONNECTTIMEOUT, 8);
			curl_setopt($ch, CURLOPT_TIMEOUT, 10);
			$data = curl_exec($ch);
			curl_close($ch);

			return $data;
		}
	}

	function is_url_from_our_site($url)
	{
		$b_our_site = stripos($url, "vladonai.com") !== false || 
			stripos($url, "allmynotes.org") !== false || 
			stripos($url, "allmynotes.info") !== false;

		return $b_our_site;
	}

	function is_invalid_str_found($str_to_search_in, $str_to_search_for, &$reason_str)
	{
		$b_found = stripos($str_to_search_in, $str_to_search_for) !== false;

		if ($b_found)
			$reason_str = "found keyword \"{$str_to_search_for}\"";

		return $b_found;
	}

	function is_url_ok_to_be_crawled($url, $link_visible_text, &$reason_str)
	{
		$parsed_url = parse_url($url);

		if ($parsed_url['scheme'] != 'https' && 
			$parsed_url['scheme'] != 'http')
		{
			$reason_str = 'no http/https = ' . $url . '===' . $parsed_url['scheme'];
			return false;
		}

		if (filter_var($parsed_url['host'], FILTER_VALIDATE_IP))
		{
			//do not crawl IPs, likely these are not even sites but some sort of playgrounds and internal networks
			$reason_str = 'no domain name, only ip';
			return false;
		}

		if (stripos($url, ".css") !== false ||
			stripos($url, ".aif") !== false ||
			stripos($url, ".mpa") !== false ||
			stripos($url, ".wma") !== false ||
			stripos($url, ".mov") !== false ||
			stripos($url, ".3gp") !== false ||
			stripos($url, ".mpg") !== false ||
			stripos($url, ".mpeg") !== false ||
			stripos($url, ".tiff") !== false ||
			stripos($url, ".tif") !== false ||
			stripos($url, ".raw") !== false ||
			stripos($url, ".qt") !== false ||
			stripos($url, ".ram") !== false ||
			stripos($url, ".txt") !== false ||
			stripos($url, ".wav") !== false ||
			stripos($url, ".ogg") !== false ||
			stripos($url, ".avi") !== false ||
			stripos($url, ".flv") !== false ||
			stripos($url, ".mp4") !== false ||
			stripos($url, ".mp3") !== false ||
			stripos($url, ".pdf") !== false ||
			stripos($url, ".doc") !== false ||
			stripos($url, ".xls") !== false ||
			stripos($url, ".csv") !== false ||
			stripos($url, ".jpeg") !== false ||
			stripos($url, ".jpg") !== false ||
			stripos($url, ".gif") !== false ||
			stripos($url, ".png") !== false ||
			stripos($url, ".ico") !== false ||
			stripos($url, ".svg") !== false ||
			stripos($url, ".psd") !== false ||
			stripos($url, ".bmp") !== false ||
			stripos($url, ".fon") !== false ||
			stripos($url, ".fnt") !== false ||
			stripos($url, ".otf") !== false ||
			stripos($url, ".ttf") !== false ||
			stripos($url, ".torrent") !== false ||
			stripos($url, ".exe") !== false ||
			stripos($url, ".bin") !== false ||
			stripos($url, ".bin") !== false ||
			stripos($url, ".bin") !== false ||
			stripos($url, ".bin") !== false ||
			stripos($url, ".bin") !== false ||
			stripos($url, ".bin") !== false ||
			stripos($url, ".ico") !== false ||
			stripos($url, ".dmp") !== false ||
			stripos($url, ".dll") !== false ||
			stripos($url, ".rss") !== false ||
			stripos($url, ".msi") !== false ||
			stripos($url, ".bin") !== false ||
			stripos($url, ".apk") !== false ||
			stripos($url, ".bin") !== false ||
			stripos($url, ".dmg") !== false ||
			stripos($url, ".toast") !== false ||
			stripos($url, ".dmg") !== false ||
			stripos($url, ".vcd") !== false ||
			stripos($url, ".iso") !== false ||
			stripos($url, ".rpm") !== false ||
			stripos($url, ".tar") !== false ||
			stripos($url, ".arj") !== false ||
			stripos($url, ".7z") !== false ||
			stripos($url, ".rar") !== false ||
			stripos($url, ".zip") !== false)
		{
			$reason_str = 'invalid file extension';
			return false; //invalid URL extension (unsupported)
		}

		$url_and_txt .= $url . $link_visible_text;

		if (stripos($url, "8500.ru") !== false)
		{
			$b_skip_url = 
				stripos($url, "/cat/") == FALSE;

			if ($b_skip_url)
			{
				$reason_str = 'URL is in explicit removal list';
				return false;
			}
		}

		if (is_url_from_our_site($url))
			return true;

		$b_skip_url = 
				is_invalid_str_found($url, "facebook", $reason_str) ||
				is_invalid_str_found($url_and_txt, "sssssssssss", $reason_str) ||
				is_invalid_str_found($url_and_txt, "sssssssssss", $reason_str) ||
				is_invalid_str_found($url_and_txt, "sssssssssss", $reason_str) ||
				is_invalid_str_found($url_and_txt, "sssssssssss", $reason_str) ||
				is_invalid_str_found($url_and_txt, "sssssssssss", $reason_str) ||
				is_invalid_str_found($url_and_txt, "sssssssssss", $reason_str) ||
				is_invalid_str_found($url_and_txt, "sssssssssss", $reason_str) ||
				is_invalid_str_found($url_and_txt, "sssssssssss", $reason_str) ||
				is_invalid_str_found($url_and_txt, "sssssssssss", $reason_str) ||
				is_invalid_str_found($url_and_txt, "sssssssssss", $reason_str) ||
				is_invalid_str_found($url_and_txt, "sssssssssss", $reason_str) ||
				is_invalid_str_found($url_and_txt, "sssssssssss", $reason_str) ||
				is_invalid_str_found($url_and_txt, "sssssssssss", $reason_str) ||
				is_invalid_str_found($url_and_txt, "sssssssssss", $reason_str) ||
				is_invalid_str_found($url_and_txt, "sssssssssss", $reason_str) ||
				is_invalid_str_found($url_and_txt, "sssssssssss", $reason_str) ||
				is_invalid_str_found($url_and_txt, "sssssssssss", $reason_str) ||
				is_invalid_str_found($url_and_txt, "sssssssssss", $reason_str) ||
				is_invalid_str_found($url_and_txt, "sssssssssss", $reason_str) ||
				is_invalid_str_found($url_and_txt, "sssssssssss", $reason_str) ||
				is_invalid_str_found($url_and_txt, "sssssssssss", $reason_str) ||
				is_invalid_str_found($url_and_txt, "sssssssssss", $reason_str) ||
				is_invalid_str_found($url_and_txt, "sssssssssss", $reason_str) ||
				is_invalid_str_found($url_and_txt, "sssssssssss", $reason_str) ||
				is_invalid_str_found($url_and_txt, "sssssssssss", $reason_str) ||
				is_invalid_str_found($url_and_txt, "sssssssssss", $reason_str) ||
				is_invalid_str_found($url_and_txt, "vogue.", $reason_str) ||
				is_invalid_str_found($url_and_txt, ".indir.com", $reason_str) ||
				is_invalid_str_found($url_and_txt, "namshi.com", $reason_str) ||
				is_invalid_str_found($url_and_txt, "apache.org", $reason_str) ||
				is_invalid_str_found($url_and_txt, "nisantasi.edu.tr", $reason_str) ||
				is_invalid_str_found($url_and_txt, "utoronto.ca", $reason_str) ||
				is_invalid_str_found($url_and_txt, "wmtransfer.com", $reason_str) ||
				is_invalid_str_found($url_and_txt, "wiktionary.org", $reason_str) ||
				is_invalid_str_found($url_and_txt, "wikiquote.org", $reason_str) ||
				is_invalid_str_found($url_and_txt, "webmoney.ru", $reason_str) ||
				is_invalid_str_found($url_and_txt, "bakeca.it", $reason_str) ||
				is_invalid_str_found($url_and_txt, "wikimediafoundation.org", $reason_str) ||
				is_invalid_str_found($url_and_txt, "wikibooks.org", $reason_str) ||
				is_invalid_str_found($url_and_txt, "wikisource.org", $reason_str) ||
				is_invalid_str_found($url_and_txt, "/docs/", $reason_str) ||
				is_invalid_str_found($url_and_txt, "/comments/", $reason_str) ||
				is_invalid_str_found($url_and_txt, "/post/", $reason_str) ||
				is_invalid_str_found($url_and_txt, "/manual/", $reason_str) ||
				is_invalid_str_found($url_and_txt, "ChangeLog-", $reason_str) ||
				is_invalid_str_found($url_and_txt, "people.php.net", $reason_str) ||
				is_invalid_str_found($url_and_txt, "richmond.edu", $reason_str) ||
				is_invalid_str_found($url_and_txt, "system1.com", $reason_str) ||
				is_invalid_str_found($url_and_txt, "howstuffworks.com", $reason_str) ||
				is_invalid_str_found($url_and_txt, "textfiles.com", $reason_str) ||
				is_invalid_str_found($url_and_txt, "wikihow.", $reason_str) ||
				is_invalid_str_found($url_and_txt, "conocophillips.", $reason_str) ||
				is_invalid_str_found($url_and_txt, "mashable.com", $reason_str) ||
				is_invalid_str_found($url_and_txt, "aol.com", $reason_str) ||
				is_invalid_str_found($url_and_txt, "bbc.com", $reason_str) ||
				is_invalid_str_found($url_and_txt, ".slb.com", $reason_str) ||
				is_invalid_str_found($url_and_txt, "uli.org", $reason_str) ||
				is_invalid_str_found($url_and_txt, "time.com", $reason_str) ||
				is_invalid_str_found($url_and_txt, "gannett.com", $reason_str) ||
				is_invalid_str_found($url_and_txt, "oracle.com", $reason_str) ||
				is_invalid_str_found($url_and_txt, "twitch.tv", $reason_str) ||
				is_invalid_str_found($url_and_txt, "crazyegg.com", $reason_str) ||
				is_invalid_str_found($url_and_txt, "adobe.com", $reason_str) ||
				is_invalid_str_found($url_and_txt, "autodesk.com", $reason_str) ||
				is_invalid_str_found($url_and_txt, "usatoday.com", $reason_str) ||
				is_invalid_str_found($url_and_txt, "imgur.com", $reason_str) ||
				is_invalid_str_found($url_and_txt, "archive.org", $reason_str) ||
				is_invalid_str_found($url_and_txt, "doubleclick", $reason_str) ||
				is_invalid_str_found($url_and_txt, "googleads", $reason_str) ||
				is_invalid_str_found($url_and_txt, "correctional", $reason_str) ||
				is_invalid_str_found($url_and_txt, "trustedmediabrands.com", $reason_str) ||
				is_invalid_str_found($url_and_txt, "stripe.com", $reason_str) ||
				is_invalid_str_found($url_and_txt, "earthlink", $reason_str) ||
				is_invalid_str_found($url_and_txt, "oclc.org", $reason_str) ||
				is_invalid_str_found($url_and_txt, "Library Information", $reason_str) ||
				is_invalid_str_found($url_and_txt, "Ask a Librarian", $reason_str) ||
				is_invalid_str_found($url_and_txt, "Online Catalog", $reason_str) ||
				is_invalid_str_found($url_and_txt, "writerfolio.com", $reason_str) ||
				is_invalid_str_found($url_and_txt, "aws.com", $reason_str) ||
				is_invalid_str_found($url_and_txt, "walmart.com", $reason_str) ||
				is_invalid_str_found($url_and_txt, "stackexchange.com", $reason_str) ||
				is_invalid_str_found($url_and_txt, "kremlin.ru", $reason_str) ||
				is_invalid_str_found($url_and_txt, "creativecommons.org", $reason_str) ||
				is_invalid_str_found($url_and_txt, "nalog.ru", $reason_str) ||
				is_invalid_str_found($url_and_txt, "nic.ru", $reason_str) ||
				is_invalid_str_found($url_and_txt, "rbc.ru", $reason_str) ||
				is_invalid_str_found($url_and_txt, "aport.ru", $reason_str) ||
				is_invalid_str_found($url_and_txt, "hit.ua", $reason_str) ||
				is_invalid_str_found($url_and_txt, "narod.ru", $reason_str) ||
				is_invalid_str_found($url_and_txt, "ucoz.", $reason_str) ||
				is_invalid_str_found($url_and_txt, "archives.ru", $reason_str) ||
				is_invalid_str_found($url_and_txt, "liveinternet.", $reason_str) ||
				is_invalid_str_found($url_and_txt, ".da.ru", $reason_str) ||
				is_invalid_str_found($url_and_txt, ".en.ee", $reason_str) ||
				is_invalid_str_found($url_and_txt, "reg.ru", $reason_str) ||
				is_invalid_str_found($url_and_txt, "wix.com", $reason_str) ||
				is_invalid_str_found($url_and_txt, "wixsite", $reason_str) ||
				is_invalid_str_found($url_and_txt, "bigmir.net", $reason_str) ||
				is_invalid_str_found($url_and_txt, "vkontakte.ru", $reason_str) ||
				is_invalid_str_found($url_and_txt, "ok.ru", $reason_str) ||
				is_invalid_str_found($url_and_txt, "vk.com", $reason_str) ||
				is_invalid_str_found($url_and_txt, "messenger.com", $reason_str) ||
				is_invalid_str_found($url_and_txt, "/m/me", $reason_str) ||
				is_invalid_str_found($url_and_txt, "government.ru", $reason_str) ||
				is_invalid_str_found($url_and_txt, "odnoklassniki.ru", $reason_str) ||
				is_invalid_str_found($url_and_txt, "github", $reason_str) ||
				is_invalid_str_found($url_and_txt, "sportcity74.ru", $reason_str) ||
				is_invalid_str_found($url_and_txt, "ucoz.ru", $reason_str) ||
				is_invalid_str_found($url_and_txt, "top-matras.ru", $reason_str) ||
				is_invalid_str_found($url_and_txt, "/reg.ru/", $reason_str) ||
				is_invalid_str_found($url_and_txt, "okmatras.ru", $reason_str) ||
				is_invalid_str_found($url_and_txt, "telegram.me", $reason_str) ||
				is_invalid_str_found($url_and_txt, "/t.me/", $reason_str) ||
				is_invalid_str_found($url_and_txt, "hotlog.ru", $reason_str) ||
				is_invalid_str_found($url_and_txt, "megagroup.ru", $reason_str) ||
				is_invalid_str_found($url_and_txt, "/wa.me/", $reason_str) ||
				is_invalid_str_found($url_and_txt, "gismeteo", $reason_str) ||
				is_invalid_str_found($url_and_txt, "whatsapp", $reason_str) ||
				is_invalid_str_found($url_and_txt, "@", $reason_str) ||
				is_invalid_str_found($url_and_txt, "ibm.com", $reason_str) ||
				is_invalid_str_found($url_and_txt, "craigslist.org", $reason_str) ||
				is_invalid_str_found($url_and_txt, "bank", $reason_str) ||
				is_invalid_str_found($url_and_txt, "piraeusbank", $reason_str) ||
				is_invalid_str_found($url_and_txt, "cloudflare.com", $reason_str) ||
				is_invalid_str_found($url_and_txt, "dmca.com", $reason_str) ||
				is_invalid_str_found($url_and_txt, "cyberbullying.org", $reason_str) ||
				is_invalid_str_found($url_and_txt, "discovery.com", $reason_str) ||
				is_invalid_str_found($url_and_txt, "patreon.com", $reason_str) ||
				is_invalid_str_found($url_and_txt, ".yelp.com", $reason_str) ||
				is_invalid_str_found($url_and_txt, "backlinko.com", $reason_str) ||
				is_invalid_str_found($url_and_txt, ".make.co", $reason_str) ||
				is_invalid_str_found($url_and_txt, "hubspot.com", $reason_str) ||
				is_invalid_str_found($url_and_txt, "slideshare.net", $reason_str) ||
				is_invalid_str_found($url_and_txt, "eepurl.com", $reason_str) ||
				is_invalid_str_found($url_and_txt, "medium.com", $reason_str) ||
				is_invalid_str_found($url_and_txt, "restored316designs.com", $reason_str) ||
				is_invalid_str_found($url_and_txt, "apartmenttherapy.com", $reason_str) ||
				is_invalid_str_found($url_and_txt, "houzz.com", $reason_str) ||
				is_invalid_str_found($url_and_txt, "spotify.com", $reason_str) ||
				is_invalid_str_found($url_and_txt, "tumblr.com", $reason_str) ||
				is_invalid_str_found($url_and_txt, "statista.com", $reason_str) ||
				is_invalid_str_found($url_and_txt, "wpengine.com", $reason_str) ||
				is_invalid_str_found($url_and_txt, "shareasale.com", $reason_str) ||
				is_invalid_str_found($url_and_txt, "blogger.com", $reason_str) ||
				is_invalid_str_found($url_and_txt, "rstyle.me", $reason_str) ||
				is_invalid_str_found($url_and_txt, "statcounter.com", $reason_str) ||
				is_invalid_str_found($url_and_txt, "feedburner.com", $reason_str) ||
				is_invalid_str_found($url_and_txt, "shopify.com", $reason_str) ||
				is_invalid_str_found($url_and_txt, "bloglovin.com", $reason_str) ||
				is_invalid_str_found($url_and_txt, "kobo.com", $reason_str) ||
				is_invalid_str_found($url_and_txt, "/t.co", $reason_str) ||
				is_invalid_str_found($url_and_txt, "/tor.com", $reason_str) || //they have protection against bots
				is_invalid_str_found($url_and_txt, "livejournal.com", $reason_str) || //likely they cannot see stats there
				is_invalid_str_found($url_and_txt, "bbc.co.", $reason_str) ||
				is_invalid_str_found($url_and_txt, "surveymonkey.com", $reason_str) ||
				is_invalid_str_found($url_and_txt, "tinyurl.com", $reason_str) ||
				is_invalid_str_found($url_and_txt, "barnesandnoble.com", $reason_str) ||
				is_invalid_str_found($url_and_txt, "bit.ly", $reason_str) ||
				is_invalid_str_found($url_and_txt, "amzn.to", $reason_str) ||
				is_invalid_str_found($url_and_txt, "wp-", $reason_str) ||
				is_invalid_str_found($url_and_txt, "wordpress", $reason_str) ||
				is_invalid_str_found($url_and_txt, "phpbb", $reason_str) ||
				is_invalid_str_found($url_and_txt, "mail", $reason_str) ||
				is_invalid_str_found($url_and_txt, "nytimes.com", $reason_str) ||
				is_invalid_str_found($url_and_txt, "guardian.", $reason_str) ||
				is_invalid_str_found($url_and_txt, "virus", $reason_str) ||
				is_invalid_str_found($url_and_txt, "security", $reason_str) ||
				is_invalid_str_found($url_and_txt, "adlock", $reason_str) ||
				is_invalid_str_found($url_and_txt, "adblock", $reason_str) ||
				is_invalid_str_found($url_and_txt, "adguard", $reason_str) ||
				is_invalid_str_found($url_and_txt, "/crt-", $reason_str) || //ex: http://repository.appvisor.com/crt-321027a54f9c
				is_invalid_str_found($url_and_txt, "wiki.appvisor.org", $reason_str) ||
				is_invalid_str_found($url_and_txt, "airforce.com", $reason_str) ||
				is_invalid_str_found($url_and_txt, "jpmorgan", $reason_str) ||
				is_invalid_str_found($url_and_txt, ".gov", $reason_str) ||
				is_invalid_str_found($url_and_txt, ".mil", $reason_str) ||
				is_invalid_str_found($url_and_txt, "ovh.", $reason_str) ||
				is_invalid_str_found($url_and_txt, "issuu", $reason_str) ||
				is_invalid_str_found($url_and_txt, "fedex", $reason_str) ||
				is_invalid_str_found($url_and_txt, "usps.com", $reason_str) ||
				is_invalid_str_found($url_and_txt, "ncaa.com", $reason_str) ||
				is_invalid_str_found($url_and_txt, "xfinity.com", $reason_str) ||
				is_invalid_str_found($url_and_txt, "coronavirus", $reason_str) ||
				is_invalid_str_found($url_and_txt, "covid", $reason_str) ||
				is_invalid_str_found($url_and_txt, "cbssports", $reason_str) ||
				is_invalid_str_found($url_and_txt, "blogspot", $reason_str) ||
				is_invalid_str_found($url_and_txt, "wsj.com", $reason_str) ||
				is_invalid_str_found($url_and_txt, "gopaytoday", $reason_str) ||
				is_invalid_str_found($url_and_txt, "evernote", $reason_str) ||
				is_invalid_str_found($url_and_txt, "onenote", $reason_str) ||
				is_invalid_str_found($url_and_txt, "curriculog", $reason_str) ||
				is_invalid_str_found($url_and_txt, "snapchat", $reason_str) ||
				is_invalid_str_found($url_and_txt, "flipboard.com", $reason_str) ||
				is_invalid_str_found($url_and_txt, "example.com", $reason_str) ||
				is_invalid_str_found($url_and_txt, "who.int", $reason_str) ||
				is_invalid_str_found($url_and_txt, "office.com", $reason_str) ||
				is_invalid_str_found($url_and_txt, "comcast", $reason_str) ||
				is_invalid_str_found($url_and_txt, "enterprise.com", $reason_str) ||
				is_invalid_str_found($url_and_txt, "www/.", $reason_str) ||
/*
				(is_invalid_str_found($url_and_txt, ".worldcat.org", $reason_str) && is_invalid_str_found($url_and_txt, "www.worldcat.org") === false) ||
				(is_invalid_str_found($url_and_txt, ".goodreads.com", $reason_str) && is_invalid_str_found($url_and_txt, "www.goodreads.com") === false) ||
				(is_invalid_str_found($url_and_txt, ".medium.com", $reason_str) && is_invalid_str_found($url_and_txt, "www.medium.com") === false) ||
				//(is_invalid_str_found($url_and_txt, ".appvisor.com", $reason_str) && is_invalid_str_found($url_and_txt, "repository.appvisor.com") === false) ||
				(is_invalid_str_found($url_and_txt, ".ag.org", $reason_str) && is_invalid_str_found($url_and_txt, "www.ag.org") === false) ||
				(is_invalid_str_found($url_and_txt, ".bncollege.com", $reason_str) && is_invalid_str_found($url_and_txt, "www.bncollege.com") === false) ||
				(is_invalid_str_found($url_and_txt, ".yale.edu", $reason_str) && is_invalid_str_found($url_and_txt, "www.yale.edu") === false) ||
				(is_invalid_str_found($url_and_txt, ".uottawa.ca", $reason_str) && is_invalid_str_found($url_and_txt, "www.uottawa.ca") === false) ||
				(is_invalid_str_found($url_and_txt, ".uqam.ca", $reason_str) && is_invalid_str_found($url_and_txt, "www.uqam.ca") === false && is_invalid_str_found($url_and_txt, "www2.uqam.ca") === false) ||
				(is_invalid_str_found($url_and_txt, ".asu.edu", $reason_str) && is_invalid_str_found($url_and_txt, "www.asu.edu") === false && is_invalid_str_found($url_and_txt, "www2.asu.edu") === false) ||
				(is_invalid_str_found($url_and_txt, ".adelphi.edu", $reason_str) && is_invalid_str_found($url_and_txt, "www.adelphi.edu") === false && is_invalid_str_found($url_and_txt, "www2.adelphi.edu") === false) ||
				(is_invalid_str_found($url_and_txt, ".auburn.edu", $reason_str) && is_invalid_str_found($url_and_txt, "www.auburn.edu") === false && is_invalid_str_found($url_and_txt, "www2.auburn.edu") === false) ||
				(is_invalid_str_found($url_and_txt, ".acadiau.ca", $reason_str) && is_invalid_str_found($url_and_txt, "www.acadiau.ca") === false && is_invalid_str_found($url_and_txt, "www2.acadiau.ca") === false) ||
*/
				is_invalid_str_found($url_and_txt, "zoom.", $reason_str) ||
				is_invalid_str_found($url_and_txt, "card", $reason_str) ||
				is_invalid_str_found($url_and_txt, "typepad.com", $reason_str) ||
				is_invalid_str_found($url_and_txt, "reuters", $reason_str) ||
				is_invalid_str_found($url_and_txt, "bloomberg", $reason_str) ||

				is_invalid_str_found($url_and_txt, "gettyimages", $reason_str) ||
				is_invalid_str_found($url_and_txt, ".ku.dk", $reason_str) ||
				is_invalid_str_found($url_and_txt, "rakuten", $reason_str) ||
				is_invalid_str_found($url_and_txt, "bloomberg", $reason_str) ||
				is_invalid_str_found($url_and_txt, "un.org", $reason_str) ||
				is_invalid_str_found($url_and_txt, "unesco", $reason_str) ||
				is_invalid_str_found($url_and_txt, "viber", $reason_str) ||
				is_invalid_str_found($url_and_txt, "account", $reason_str) ||
				is_invalid_str_found($url_and_txt, "nark", $reason_str) ||
				is_invalid_str_found($url_and_txt, "doza", $reason_str) ||
				is_invalid_str_found($url_and_txt, "nestle", $reason_str) ||
				is_invalid_str_found($url_and_txt, "mastercard", $reason_str) ||
				is_invalid_str_found($url_and_txt, "visa", $reason_str) ||
				is_invalid_str_found($url_and_txt, "portmone", $reason_str) ||
				is_invalid_str_found($url_and_txt, "whatsapp.com", $reason_str) ||
				is_invalid_str_found($url_and_txt, "novaposhta", $reason_str) ||
				is_invalid_str_found($url_and_txt, "booking", $reason_str) ||
				is_invalid_str_found($url_and_txt, ".gov.ua", $reason_str) ||
				is_invalid_str_found($url_and_txt, "spam", $reason_str) ||
				is_invalid_str_found($url_and_txt, "coinbase", $reason_str) ||
				is_invalid_str_found($url_and_txt, "comodo", $reason_str) ||
				is_invalid_str_found($url_and_txt, "secure.", $reason_str) ||
				is_invalid_str_found($url_and_txt, "avast", $reason_str) ||
				is_invalid_str_found($url_and_txt, "avira", $reason_str) ||
				is_invalid_str_found($url_and_txt, "crowdstrike", $reason_str) ||
				is_invalid_str_found($url_and_txt, "secureanywhere", $reason_str) ||
				is_invalid_str_found($url_and_txt, "Webroot", $reason_str) ||
				is_invalid_str_found($url_and_txt, "carbon", $reason_str) ||
				is_invalid_str_found($url_and_txt, "VIPRE", $reason_str) ||
				is_invalid_str_found($url_and_txt, "McAfee", $reason_str) ||
				is_invalid_str_found($url_and_txt, "Malware", $reason_str) ||
				is_invalid_str_found($url_and_txt, "BullGuard", $reason_str) ||
				is_invalid_str_found($url_and_txt, "bitdefender", $reason_str) ||
				is_invalid_str_found($url_and_txt, "avg", $reason_str) ||
				is_invalid_str_found($url_and_txt, "trendmicro", $reason_str) ||
				is_invalid_str_found($url_and_txt, "sophos", $reason_str) ||
				is_invalid_str_found($url_and_txt, "norton", $reason_str) ||
				is_invalid_str_found($url_and_txt, "panda", $reason_str) ||
				is_invalid_str_found($url_and_txt, "eset.", $reason_str) ||
				is_invalid_str_found($url_and_txt, "kaspersky", $reason_str) ||
				is_invalid_str_found($url_and_txt, "admin.", $reason_str) ||
				is_invalid_str_found($url_and_txt, "mk.ru", $reason_str) ||
				is_invalid_str_found($url_and_txt, "aif.ru", $reason_str) ||
				is_invalid_str_found($url_and_txt, "iphone", $reason_str) ||
				is_invalid_str_found($url_and_txt, "linux", $reason_str) ||
				is_invalid_str_found($url_and_txt, "apple", $reason_str) ||
				is_invalid_str_found($url_and_txt, "huawei.", $reason_str) ||
				is_invalid_str_found($url_and_txt, ".i.ua", $reason_str) ||
				is_invalid_str_found($url_and_txt, "ebay.", $reason_str) ||
				is_invalid_str_found($url_and_txt, "mail.ru", $reason_str) ||
				is_invalid_str_found($url_and_txt, "w3.org", $reason_str) ||
				is_invalid_str_found($url_and_txt, "likee", $reason_str) ||
				is_invalid_str_found($url_and_txt, "tiktok", $reason_str) ||
				is_invalid_str_found($url_and_txt, "police", $reason_str) ||
				is_invalid_str_found($url_and_txt, "court", $reason_str) ||
				is_invalid_str_found($url_and_txt, ".gov", $reason_str) ||
				is_invalid_str_found($url_and_txt, "press", $reason_str) ||
				is_invalid_str_found($url_and_txt, "privacy", $reason_str) ||
				is_invalid_str_found($url_and_txt, "advertising", $reason_str) ||
				is_invalid_str_found($url_and_txt, "qq.com", $reason_str) ||
				is_invalid_str_found($url_and_txt, "pmc.com", $reason_str) ||
				is_invalid_str_found($url_and_txt, "api.", $reason_str) ||
				is_invalid_str_found($url_and_txt, "subscribe", $reason_str) ||
				is_invalid_str_found($url_and_txt, "taobao", $reason_str) ||
				is_invalid_str_found($url_and_txt, "aliexpress", $reason_str) ||
				is_invalid_str_found($url_and_txt, "android", $reason_str) || //we do not support android yet
				is_invalid_str_found($url_and_txt, "ios", $reason_str) || //we do not support iOS yet
				is_invalid_str_found($url_and_txt, "linux", $reason_str) || //we do not support Linux yet
				is_invalid_str_found($url_and_txt, "macos", $reason_str) || //we do not support Mac yet
				is_invalid_str_found($url_and_txt, "mac os", $reason_str) || //we do not support Mac yet
				is_invalid_str_found($url_and_txt, "itunes", $reason_str) ||
				is_invalid_str_found($url_and_txt, "opera.", $reason_str) ||
				is_invalid_str_found($url_and_txt, "mozilla.", $reason_str) ||
				is_invalid_str_found($url_and_txt, "cnet.", $reason_str) ||
				is_invalid_str_found($url_and_txt, "TYPO3", $reason_str) ||
				is_invalid_str_found($url_and_txt, "phpbb", $reason_str) ||
				is_invalid_str_found($url_and_txt, "cms", $reason_str) ||
				is_invalid_str_found($url_and_txt, "Joomla", $reason_str) ||
				is_invalid_str_found($url_and_txt, "bbPress", $reason_str) ||
				is_invalid_str_found($url_and_txt, "vBulletin", $reason_str) ||
				is_invalid_str_found($url_and_txt, "phpmyadmin", $reason_str) ||
				is_invalid_str_found($url_and_txt, "manage.", $reason_str) ||
				is_invalid_str_found($url_and_txt, "hosting", $reason_str) ||
				is_invalid_str_found($url_and_txt, "vps", $reason_str) ||
				is_invalid_str_found($url_and_txt, "vpn", $reason_str) ||
				is_invalid_str_found($url_and_txt, "dns", $reason_str) ||
				is_invalid_str_found($url_and_txt, "microsoft.com", $reason_str) ||
				is_invalid_str_found($url_and_txt, "vimeo", $reason_str) ||
				is_invalid_str_found($url_and_txt, "download.com", $reason_str) ||
				is_invalid_str_found($url_and_txt, "evernote", $reason_str) ||
				is_invalid_str_found($url_and_txt, "cdn", $reason_str) ||
				is_invalid_str_found($url_and_txt, "domain", $reason_str) ||
				is_invalid_str_found($url_and_txt, "softonic.", $reason_str) ||
				is_invalid_str_found($url_and_txt, "whois", $reason_str) ||
				is_invalid_str_found($url_and_txt, "porno", $reason_str) ||
				is_invalid_str_found($url_and_txt, "sex", $reason_str) ||
				is_invalid_str_found($url_and_txt, "adult", $reason_str) ||
				is_invalid_str_found($url_and_txt, "viagra", $reason_str) ||
				is_invalid_str_found($url_and_txt, "abuse", $reason_str) ||
				is_invalid_str_found($url_and_txt, "bdsm", $reason_str) ||
				is_invalid_str_found($url_and_txt, "emerge", $reason_str) ||
				is_invalid_str_found($url_and_txt, "covid", $reason_str) ||
				//is_invalid_str_found($url_and_txt, "vladonai.com", $reason_str) ||
				//is_invalid_str_found($url_and_txt, "allmynotes.org", $reason_str) ||
				//is_invalid_str_found($url_and_txt, "allmynotes.info", $reason_str) ||
				is_invalid_str_found($url_and_txt, "icann.", $reason_str) ||
				is_invalid_str_found($url_and_txt, "linode.com", $reason_str) ||
				is_invalid_str_found($url_and_txt, "paypal.com", $reason_str) ||
				is_invalid_str_found($url_and_txt, "arin.net", $reason_str) ||
				is_invalid_str_found($url_and_txt, "afrinic.net", $reason_str) ||
				is_invalid_str_found($url_and_txt, "lacnic.net", $reason_str) ||
				is_invalid_str_found($url_and_txt, "domains", $reason_str) ||
				is_invalid_str_found($url_and_txt, "instagram", $reason_str) ||
				is_invalid_str_found($url_and_txt, "google", $reason_str) ||
				is_invalid_str_found($url_and_txt, "goo.gl", $reason_str) ||
				is_invalid_str_found($url_and_txt, "twitter", $reason_str) ||
				is_invalid_str_found($url_and_txt, "yahoo", $reason_str) ||
				is_invalid_str_found($url_and_txt, "linkedin", $reason_str) ||
				is_invalid_str_found($url_and_txt, "baidu", $reason_str) ||
				is_invalid_str_found($url_and_txt, "amazon.", $reason_str) ||
				is_invalid_str_found($url_and_txt, "Wikipedia.org", $reason_str) ||
				is_invalid_str_found($url_and_txt, "Live.com", $reason_str) ||
				is_invalid_str_found($url_and_txt, "Reddit.com", $reason_str) ||
				is_invalid_str_found($url_and_txt, "Netflix.com", $reason_str) ||
				is_invalid_str_found($url_and_txt, "Bing.com", $reason_str) ||
				is_invalid_str_found($url_and_txt, "Ebay.com", $reason_str) ||
				is_invalid_str_found($url_and_txt, "Aliexpress.", $reason_str) ||
				is_invalid_str_found($url_and_txt, "Apple.com", $reason_str) ||
				is_invalid_str_found($url_and_txt, "Yandex.ru", $reason_str) ||
				is_invalid_str_found($url_and_txt, "cnn.com", $reason_str) ||
				is_invalid_str_found($url_and_txt, "chase", $reason_str) ||
				is_invalid_str_found($url_and_txt, "citi", $reason_str) ||
				is_invalid_str_found($url_and_txt, "etsy.", $reason_str) ||
				is_invalid_str_found($url_and_txt, "dropbox.", $reason_str) ||
				is_invalid_str_found($url_and_txt, "gmail.", $reason_str) ||
				is_invalid_str_found($url_and_txt, "msn.com", $reason_str) ||
				is_invalid_str_found($url_and_txt, "imdb.", $reason_str) ||
				is_invalid_str_found($url_and_txt, "wordpress.", $reason_str) ||
				is_invalid_str_found($url_and_txt, "godaddy.com", $reason_str) ||
				is_invalid_str_found($url_and_txt, "webmail", $reason_str) ||
				is_invalid_str_found($url_and_txt, "pinterest", $reason_str) ||
				is_invalid_str_found($url_and_txt, "flickr.", $reason_str) ||
				is_invalid_str_found($url_and_txt, ".pdf", $reason_str) ||
				is_invalid_str_found($url_and_txt, "generatepress.", $reason_str) ||
				is_invalid_str_found($url_and_txt, "concept3d.com", $reason_str) ||
				is_invalid_str_found($url_and_txt, "gotoassist.com", $reason_str) ||
				is_invalid_str_found($url_and_txt, "o365.", $reason_str) ||
				is_invalid_str_found($url_and_txt, "login", $reason_str) ||
				is_invalid_str_found($url_and_txt, "/search", $reason_str) ||
				is_invalid_str_found($url_and_txt, "moodle", $reason_str) ||
				is_invalid_str_found($url_and_txt, "youtu.be", $reason_str) ||
				is_invalid_str_found($url_and_txt, "youtube", $reason_str); 

		return !$b_skip_url;
	}

	function CrawlNewUrl(&$config_file_data, $single_site_to_handle_id, $task_name, $extra_params, &$cumulative_list_of_new_sites_added, $b_ref_only = false)
	{
		if (!array_key_exists($single_site_to_handle_id, $config_file_data[ENTRY_SITES_LIST]))
		{
			if (!$b_cron_task)
				echo "ERROR: invalid key {$single_site_to_handle_id}<br>";
	
			return false;
		}

		$cur_db_record = &$config_file_data[ENTRY_SITES_LIST][$single_site_to_handle_id];

		global $b_test_mode; //enable it to debug
		global $recent_app_ver_major;
		global $recent_app_ver_minor; 
		global $b_cron_task;

		//tmp, for debugging:
		if (!$b_cron_task)
			echo "<br><b>Starting</b> fetch task " . $task_name . ", site " . $cur_db_record[ENTRY_SITE_ID] . ' [' . $cur_db_record[ENTRY_ENTRY_PAGE_URL] . ']<br>';

		$url_to_fetch = $cur_db_record[ENTRY_ENTRY_PAGE_URL];
		$parsed_page_url = parse_url($url_to_fetch);
		$parsed_page_hostname = $parsed_page_url['host'];
		$b_crawl_whole_domain = $cur_db_record[ENTRY_B_CRAWL_WHOLE_DOMAIN];

		$b_restrict_url_keywords = $cur_db_record[ENTRY_ALLOWED_URL_KEYWORDS] != "";
		if ($b_restrict_url_keywords)
			$allowed_URL_keywords_array = explode(";", $cur_db_record[ENTRY_ALLOWED_URL_KEYWORDS]);

		if (stripos($url_to_fetch, "appvisor") !== false ||
			stripos($url_to_fetch, "goodreads.com") !== false ||
			stripos($url_to_fetch, "worldcat.org") !== false)
		{
			$referer = '';
		} else
		{
			$referer = REF_URL_TEMPLATE; //ex: https://www.vladonai.com/cms-bookmarks-panel?url=[url]&page=add-link-to-site
			$referer = str_replace("[url]", urlencode($url_to_fetch), $referer);
		}

		$fetch_start_time = microtime_float();
		$page_html_content = file_get_contents_for_crawling($url_to_fetch, $referer, TRUE, $b_ref_only);
		$current_microtime = microtime_float();
		$fetching_time_seconds = sprintf("%.1f", $current_microtime - $fetch_start_time);

		$b_html_content = stripos($page_html_content, "<html") !== false;
		$b_xml_content = stripos($page_html_content, "<?xml") !== false;
		$b_error_string_detected = detect_errors_codes_or_unwanted_keywords_on_page($page_html_content);
		$b_success = /*$b_ref_only ||*/ !$b_error_string_detected && (stripos($page_html_content, " 200 ") !== false || $b_html_content || $b_xml_content);

		$cur_db_record[ENTRY_B_SITE_CRAWLED] = true;
		$cur_db_record[ENTRY_STAUTS] = $b_success ? "ok" : ($b_error_string_detected ? "err - 404" : "err - timeout"); //ex values: loaded, err, skip
		$cur_db_record[ENTRY_FETCH_TIME] = $fetching_time_seconds;

		if ($b_success)
		{
			if (!$b_cron_task && $referer != "")
				echo 'Ref poked for: ' . $url_to_fetch . ' +++ ' . $referer . '<br>';
			if (!$b_cron_task && $referer != "")
				echo "Fetch time: <b>{$fetching_time_seconds} seconds</b><br>";

			if (!$b_ref_only)
			{
				//parse the fetched page
				$found_domains_list = array();

				if ($b_html_content)
				{
					$dom = new DOMDocument;
					@$dom->loadHTML($page_html_content);
					$links = $dom->getElementsByTagName('a');
				} else
				{
					$startPos = stripos($page_html_content, '<Application_Info_URL>');
					$endPos = stripos($page_html_content, '</', $startPos);
					$links = array();
					$url_to_extract = "";
					if ($startPos > 0 && $endPos > 0)
					{
						$startPos = stripos($page_html_content, '>', $startPos) + 1;
						$url_to_extract = substr($page_html_content, $startPos, $endPos - $startPos);
						$links[0] = $url_to_extract;
					}

					if ($url_to_extract == "")
						$cumulative_list_of_new_sites_added .= "---- no urls found on XML file " . $link_url . " ";
				}

				foreach ($links as $link)
				{
					if ($b_html_content)
					{
						$link_visible_text = $link->nodeValue;
						$link_url = $link->getAttribute('href');
					} else
					{
						$link_visible_text = "";
						$link_url = $link;
					}
					if (stripos($link_url, 'https://') === false &&
						 stripos($link_url, 'http://') === false)
					{
						//add domain
						$parsed_link_url_parent = parse_url($url_to_fetch);
						if ($link_url[0] != '/')
							$link_url = '/' . $link_url;
						if ($link_url[0] == '?')
							$link_url = $parsed_link_url_parent['scheme'] . '://' . $parsed_link_url_parent['host'] . $parsed_link_url_parent['path'] . $link_url;
						else
							$link_url = $parsed_link_url_parent['scheme'] . '://' . $parsed_link_url_parent['host'] . $link_url;
					}
					$link_url = str_replace('/./', '/', $link_url);

					$parsed_link_url = parse_url($link_url);
					$hostname = $parsed_link_url['host'];
					$hostname = str_replace("www.", "", $hostname);
					$hostname = strtolower($hostname);

					if ($hostname == 'www.goodreads.com' &&
						stripos($link_url, '?page=') === false)
					{
						//strip-off params
						$param_pos = strpos($link_url, '?');
						if ($param_pos > 0)
						{
							$link_url = substr($link_url, 0, $param_pos);
						}
					}

					if ($b_restrict_url_keywords)
					{
						$b_skip_due_keywords_missing = true;
						foreach ($allowed_URL_keywords_array as $url_key => $allowed_URL_keyword)
						{
							if (stripos($link_url, $allowed_URL_keyword) !== false)
							{
								$b_skip_due_keywords_missing = false;
								break;
							}
						}
					} else
					{
						$b_skip_due_keywords_missing = false;
					}

					$skip_reason_str = "";
					if (!is_url_ok_to_be_crawled($link_url, $link_visible_text, $skip_reason_str) ||
						stripos($link_url, 'goodreads.com') !== false) //tmp
					{
						//skip
						if (!$b_cron_task)
							echo '<div style="color:lightgray;">Skipped [com]: ' . $hostname . '<br>&nbsp;&nbsp;&nbsp;&nbsp;' . $link_visible_text . ' === ' . $link_domain . ' +++ ' . $link_url . ', reason: <b>' . $skip_reason_str . '</b></div><br>';
					} else
					if ($parsed_link_url['scheme'] != "" && 
						$hostname != "")
					{
						$link_domain = $parsed_link_url['scheme'] . '://' . $hostname;
						$b_add_new_domain = !array_key_exists($hostname, $config_file_data[ENTRY_SITES_LIST]);
						$b_already_listad_as_addon = stripos($config_file_data[ENTRY_SITES_LIST][str_replace('www.', '', $hostname)]["addon_urls"], $link_url) !== false;
						$b_addon_page = !$b_add_new_domain &&
										$b_crawl_whole_domain && 
										str_replace('www.', '', $hostname) == str_replace('www.', '', $parsed_page_hostname) && 
										!$b_already_listad_as_addon;
						if ($b_add_new_domain ||
							$b_addon_page)
						{
							//new domain, add it to the list
							$new_entry = array();
							$new_entry[ENTRY_SITE_ID] = strtolower($hostname) . ($b_addon_page ? (' ' . (++ $config_file_data[ENTRY_SITES_LIST][$hostname]["addon_urls_count"])) : '');
							$new_entry[ENTRY_ENTRY_PAGE_URL] = $link_url;

							if ($b_skip_due_keywords_missing)
								$new_depth = 10000; //still add the URL, but do not crawl its content, lets reach it using huge depth value
							else
								$new_depth = is_numeric($cur_db_record[ENTRY_DEPTH]) ? ((int)$cur_db_record[ENTRY_DEPTH] + ($b_addon_page ? 0 : 1)) : 1;

							$new_entry[ENTRY_DEPTH] = $new_depth;
							$new_entry[ENTRY_CRAWL_DATE] = time();
							if ($b_crawl_whole_domain && str_replace('www.', '', $hostname) == str_replace('www.', '', $parsed_page_hostname))
								$new_entry[ENTRY_B_CRAWL_WHOLE_DOMAIN] = true;

							$config_file_data[ENTRY_SITES_LIST][$new_entry[ENTRY_SITE_ID]] = $new_entry;

							if ($b_addon_page)
								$config_file_data[ENTRY_SITES_LIST][$hostname]["addon_urls"] .= "[" . $link_url . "]";

							if (!$b_cron_task)
								echo '<div style="color:green;"><b>Added</b>: ' . $link_visible_text . ' === ' . $link_domain . ' +++ ' . $link_url . '</div><br>';
							$cumulative_list_of_new_sites_added .= "[" . $link_domain . "] ";
						} else
						{
							if (!array_key_exists($hostname, $found_domains_list))
							{
								//increase link counter only once a page, this way we'll have number of links form various sites
								$found_domains_list[$hostname] = $hostname;
								$config_file_data[ENTRY_SITES_LIST][$hostname]["links_found"] += 1;
							}

							if (!$b_cron_task)
							{
								if ($skip_reason_str == '')
									$skip_reason_str = $b_already_listad_as_addon ? "+already added+" : 
													   ("addon: " . ($b_addon_page ? "true" : "FALSE") . 
													    ", add new " . ($b_add_new_domain ? "TRUE" : "FALSE") . 
													    ", crawl whole: " . ($b_crawl_whole_domain  ? "TRUE" : "FALSE") . 
													    ", " . str_replace('www.', '', $hostname) . ' vs ' . str_replace('www.', '', $parsed_page_hostname));
								echo '<div style="color:gray;">Skipped: ' . $link_visible_text . ' === ' . $link_domain . ' +++ ' . $link_url . ', reason: <b>' . $skip_reason_str . '</b></div><br>';
							}
						}
					}
				}
			}

			if ($referer != "")
			{
				$cur_db_record[ENTRY_REFS] = (int)$cur_db_record[ENTRY_REFS] + 1;
			}
		} else
		{
			if (!$b_cron_task)
				echo "err! {$url_to_fetch} <code>000begin " . htmlspecialchars(substr($page_html_content, 0, 200)) . ' 000end</code><br>';
		}

		return $b_success;
	}

	function get_hardcoded_site_repositories_list($task_name)
	{
		if ($task_name == "task_crawler_1")
		{
			//World libraries. Ex: https://www.worldcat.org/libraries/12002
			$hardcoded_sites_list = array(
				array(ENTRY_SITE_ID => "firstsiteguide.com", 
					  ENTRY_ENTRY_PAGE_URL => "https://firstsiteguide.com/examples-of-blogs/",
					  ENTRY_NOTES => "root",
					  ),
				array(ENTRY_SITE_ID => "alastairhumphreys.com", 
					  ENTRY_ENTRY_PAGE_URL => "https://alastairhumphreys.com/favourite-blogs/",
					  ENTRY_NOTES => "root",
					  ),
				array(ENTRY_SITE_ID => "buffer.com", 
					  ENTRY_ENTRY_PAGE_URL => "https://buffer.com/resources/favorite-blogs-marketing-social-media-productivity/",
					  ENTRY_NOTES => "root",
					  ),
				array(ENTRY_SITE_ID => "contentmarketinginstitute.com", 
					  ENTRY_ENTRY_PAGE_URL => "https://contentmarketinginstitute.com/top-content-marketing-blogs/",
					  ENTRY_NOTES => "root",
					  ),
				);
		} else
		if ($task_name == "task_crawler_2")
		{
			$hardcoded_sites_list = array(
				array(ENTRY_SITE_ID => "readyartwork.com", 
					  ENTRY_ENTRY_PAGE_URL => "https://www.readyartwork.com/differences-among-informational-e-commerce-e-catalog-websites/",
					  ENTRY_NOTES => "root",
					  ENTRY_B_CRAWL_WHOLE_DOMAIN => "true", 
					  ),
				);
		} else
		if ($task_name == "task_crawler_3")
		{
			$hardcoded_sites_list = array(
				array(ENTRY_SITE_ID => "www.goodreads.com MAIN Entry", 
					  ENTRY_ENTRY_PAGE_URL => "https://axiom.us.com/2018/12/40-addictive-websites-to-waste-the-day-away/",
					  ENTRY_B_CRAWL_WHOLE_DOMAIN => "true", 
					  ENTRY_NOTES => "",
					  ),
			);
		} else
		if ($task_name == "task_crawler_4")
		{
			$hardcoded_sites_list = array(
				array(ENTRY_SITE_ID => "www.nownovel.com", 
					  ENTRY_ENTRY_PAGE_URL => "https://www.nownovel.com/blog/200-best-writing-websites/",
					  ENTRY_B_CRAWL_WHOLE_DOMAIN => "true", 
					  ENTRY_NOTES => "root",
					  ),

//				array(ENTRY_SITE_ID => "CA counties list", 
//					  ENTRY_ENTRY_PAGE_URL => "https://www.counties.org/county-websites-profile-information",
//					  ENTRY_NOTES => "",
//					  ),
			);
		} else
		if ($task_name == "task_crawler_5")
		{
			$hardcoded_sites_list = array(
				array(ENTRY_SITE_ID => "vladonai.com",
					  ENTRY_ENTRY_PAGE_URL => "http://www.vladonai.com/",
					  ENTRY_NOTES => "www.vladonai.com",
					  ENTRY_B_CRAWL_WHOLE_DOMAIN => "true",
					  ENTRY_ALLOWED_URL_KEYWORDS => "www.vladonai.com;allmynotes.org;allmynotes.info",
					  ),
				array(ENTRY_SITE_ID => "allmynotes.org",
					  ENTRY_ENTRY_PAGE_URL => "https://www.allmynotes.org/",
					  ENTRY_NOTES => "www.allmynotes.org",
					  ENTRY_B_CRAWL_WHOLE_DOMAIN => "true",
					  ENTRY_ALLOWED_URL_KEYWORDS => "vladonai.com;allmynotes.org;allmynotes.info",
					  ),
				);
/*template:
				array(ENTRY_SITE_ID => "",
					  ENTRY_ENTRY_PAGE_URL => "",
					  ),

				opt:
					  "pattern_of_pgs_to_load" => "catalogue", //this word must be present
					  ENTRY_STAUTS => "", //ex values: loaded, err, skip
					  ENTRY_NOTES => "",
					  ENTRY_B_CRAWL_WHOLE_DOMAIN => "true", 
					  ENTRY_ALLOWED_URL_KEYWORDS => "vladonai.com;allmynotes.org;allmynotes.info",
					  "" => "", 
*/
		} else
		if ($task_name == "task_crawler_6")
		{
			$hardcoded_sites_list = array(
				array(ENTRY_SITE_ID => "www.debian.org", 
					  ENTRY_ENTRY_PAGE_URL => "https://www.debian.org/",
					  ENTRY_NOTES => "root",
					  ENTRY_B_CRAWL_WHOLE_DOMAIN => "true", 
					  ),
/*				array(ENTRY_SITE_ID => "codeguru.com", 
					  ENTRY_ENTRY_PAGE_URL => "https://www.codeguru.com/",
					  ENTRY_NOTES => "root",
					  ENTRY_B_CRAWL_WHOLE_DOMAIN => "true", 
					  ),
				array(ENTRY_SITE_ID => "codeproject.com", 
					  ENTRY_ENTRY_PAGE_URL => "https://www.codeproject.com/",
					  ENTRY_NOTES => "root",
					  ENTRY_B_CRAWL_WHOLE_DOMAIN => "true", 
					  ),
*/				);
		} else
		if ($task_name == "task_crawler_7")
		{
			$hardcoded_sites_list = array(
				array(ENTRY_SITE_ID => "www.php.net", 
					  ENTRY_ENTRY_PAGE_URL => "https://www.php.net/",
					  ENTRY_NOTES => "root",
					  ENTRY_B_CRAWL_WHOLE_DOMAIN => "true")
				);
		} else
		if ($task_name == "task_crawler_8")
		{
			$hardcoded_sites_list = array(
				array(ENTRY_SITE_ID => "habr.com", 
					  ENTRY_ENTRY_PAGE_URL => "https://habr.com/ru/all/",
					  ENTRY_NOTES => "root",
					  ENTRY_B_CRAWL_WHOLE_DOMAIN => "true")
					);
		} else
		if ($task_name == "task_crawler_9")
		{
			//World libraries. Ex: https://www.worldcat.org/libraries/12002
			$hardcoded_sites_list = array();
			for ($libN = 100001; $libN < 109999; $libN ++)
			{
				$new_entry = array();
				$link_url = 'https://www.worldcat.org/libraries/' . $libN;
				$new_entry[ENTRY_SITE_ID] = 'Cat ' . $libN;
				$new_entry[ENTRY_ENTRY_PAGE_URL] = $link_url;
				$new_entry[ENTRY_DEPTH] = 1;
				$hardcoded_sites_list[$new_entry[ENTRY_SITE_ID]] = $new_entry;
			};
		} else
		if ($task_name == "task_crawler_10")
		{
			//World libraries. Ex: https://www.worldcat.org/libraries/12002
			$hardcoded_sites_list = array();
			for ($libN = 110000; $libN < 119999; $libN ++)
			{
				$new_entry = array();
				$link_url = 'https://www.worldcat.org/libraries/' . $libN;
				$new_entry[ENTRY_SITE_ID] = 'Cat ' . $libN;
				$new_entry[ENTRY_ENTRY_PAGE_URL] = $link_url;
				$new_entry[ENTRY_DEPTH] = 1;
				$hardcoded_sites_list[$new_entry[ENTRY_SITE_ID]] = $new_entry;
			};
		}

		return $hardcoded_sites_list;
	}

	
	///////////////////////////////////
	//	end of page
	echo '<br>See also:<br>
		  <a href="https://my_domain.org/phpmyadm_panel" target="_blank">phpMyAdmin</a><br>
		  <a href="https://www.my_domain.com/cp_url_fetcher.php" target="_blank">URL Fetcher</a><br>
		  <br>';

	ControlPanel_RenderPrintPageEnd();
?>
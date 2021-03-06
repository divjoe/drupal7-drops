<?php

define('DEVCONNECT_ANALYTICS_MAX_XAXIS_LINES', 12);
define('DEVCONNECT_ANALYTICS_MAX_YAXIS_LINES', 10);

/**
 * Returns a "singleton" of the analytics object, or rather,
 * the closest PHP can come to a singleton.
 *
 * @return Apigee\ManagementAPI\DeveloperAppAnalytics
 */
function &_devconnect_developer_apps_analytics_api_client() {
  static $dca;

  if (!isset($dca)) {
    $client = devconnect_default_org_config();
    $environment = variable_get('devconnect_analytics_environment', 'prod');
    $dca = new Apigee\ManagementAPI\DeveloperAppAnalytics($client, $environment);
  }
  return $dca;
}

function _devconnect_developer_apps_analytics_metrics() {
  return array(
    'total_response_time' => t('Throughput'),
    'max_response_time' => t('Max Response Time'),
    'min_response_time' => t('Min Response Time'),
    'message_count' => t('Message Count'),
    'error_count' => t('Error Count'),
  );
}

function devconnect_developer_apps_analytics_form() {
  $form['#attributes']['class'] = array('form-inline', 'analytics-form');

  $metric_options = _devconnect_developer_apps_analytics_metrics();
  $metric_default = (isset($_GET['metric']) && isset($metric_options[$_GET['metric']]) ? $_GET['metric'] : 'total_response_time');

  $form['metric'] = array(
    '#type' => 'select',
    '#title' => t('Display'),
    '#required' => TRUE,
    '#multiple' => FALSE,
    '#options' => $metric_options,
    '#default_value' => $metric_default
  );

  $date_format = variable_get('date_format_short', 'm/d/Y - H:i');
  $date_format = str_replace(' - ', ' ', $date_format);

  $from = 0;
  if (isset($_GET['from'])) {
    $from = $_GET['from'];
  }
  if (!$from) {
    $from = strtotime('-1 day');
  }
  $from_text = gmdate($date_format, $from);
  $to = 0;
  if (isset($_GET['to'])) {
    $to = $_GET['to'];
  }
  if (!$to) {
    $to = time();
  }
  $to_text = gmdate($date_format, $to);

  drupal_add_library('system', 'ui.tabs');

  $timepicker_path = libraries_get_path('datetimepicker') . '/';
  drupal_add_js($timepicker_path . 'build/jquery.datetimepicker.full.min.js');
  drupal_add_css($timepicker_path . 'jquery.datetimepicker.css');
  $css_options = array(
    'group' => CSS_THEME,
    'weight' => 9999,
  );
  drupal_add_css(drupal_get_path('module', 'devconnect_developer_apps') . '/css/timepicker.css', $css_options);
  drupal_add_js("jQuery(document).ready(function(){
    jQuery('#edit-date-from').datetimepicker({format:'$date_format',step:5});
    jQuery('#edit-date-to').datetimepicker({format:'$date_format',step:5});
  });", 'inline');


  $form['date_from'] = array(
    '#type' => 'textfield',
    '#title' => t('From'),
    '#required' => TRUE,
    '#size' => 25,
    '#default_value' => $from_text,
    '#attributes' => array('class' => array('pickadate')),
    '#post_render' => array('devconnect_developer_apps_analytics_date_postrender'),
  );

  $form['date_to'] = array(
    '#type' => 'textfield',
    '#title' => t('To'),
    '#required' => TRUE,
    '#size' => 25,
    '#default_value' => $to_text,
    '#attributes' => array('class' => array('pickadate')),
    '#post_render' => array('devconnect_developer_apps_analytics_date_postrender'),
  );

  $form['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Submit'),
  );

  return $form;
}

function devconnect_developer_apps_analytics_date_postrender($element, $children) {
  return str_replace('</div>', '<span class="utc">' . t('UTC') . '</span></div>', $element);
}

function devconnect_developer_apps_analytics_form_validate($form, &$form_state) {
  $from = $form_state['values']['date_from'];
  $to = $form_state['values']['date_to'];

  $date_error = FALSE;

  if (strpos($from, 'UTC') === FALSE) {
    $from .= ' UTC';
  }
  if (strpos($to, 'UTC') === FALSE) {
    $to .= ' UTC';
  }

  $parsed_from = @strtotime($from);
  $parsed_to = @strtotime($to);

  if ($parsed_from === FALSE) {
    form_set_error('date_from', t('Invalid "From" date.'));
    $date_error = TRUE;
  }
  if ($parsed_to === FALSE) {
    form_set_error('date_to', t('Invalid "To" date.'));
    $date_error = TRUE;
  }
  if (!$date_error) {
    if ($parsed_from >= $parsed_to) {
      form_set_error('date_from', '"From" date must be earlier than "To" date.');
      $date_error = TRUE;
    }
  }
  if (!$date_error) {
    $form_state['storage']['from'] = $parsed_from;
    $form_state['storage']['to'] = $parsed_to;
  }
}

function devconnect_developer_apps_analytics_form_submit($form, &$form_state) {
  $from = $form_state['storage']['from'];
  $to = $form_state['storage']['to'];
  $metric = $form_state['values']['metric'];

  $unit = 'day';
  // Set the unit to hours only if the selected range is less than or equal to
  // 24 hours i.e less than or equal to 24*60*60 = 86400 seconds.
  $range = ($to - $from);
  if ($range <= 86400) {
    $unit = 'hour';
  }

  $args = array('from' => $from, 'to' => $to, 'metric' => $metric, 'unit' => $unit);
  $form_state['redirect'] = array(current_path(), array('query' => $args, 'fragment' => 'performance'));
}

function devconnect_developer_apps_analytics_performance($user, $app_id, $app_name = NULL) {
  $entities = entity_load('developer_app', array($app_id));
  $entity = reset($entities);
  $app_name = $entity->name;
  $my_data = _devconnect_developer_apps_analytics_get_datapoints($user, $app_name);
  if ($my_data === FALSE) {
    return '<p>' . t('An error occurred while fetching your data.') . '</p>';
  }
  $datapoints = $my_data['datapoints'];
  $date_format = $my_data['date_format'];
  $default_datapoints = $my_data['default_datapoints'];
  $time_points = $my_data['time_points'];

  $metric_fullname = $my_data['params']['metric']['display_name'];
  $metric_name = $my_data['params']['metric']['name'];

  if (empty($datapoints) || !isset($datapoints[$app_name])) {
    // TODO: handle this differently?
    return '';
  }
  if (variable_get('devconnect_highcharts_use_cdn', 'cdn') == 'cdn') {
    $highcharts_base_url = '//code.highcharts.com/3.0.10';
    $type = 'external';
  }
  else {
    $highcharts_base_url = url(libraries_get_path('highcharts'));
    $type = 'file';
  }
  drupal_add_js($highcharts_base_url . '/highcharts.js', array('type' => $type, 'group' => JS_LIBRARY));

  $chart_data = array(
    'chart' => array(
      'renderTo' => 'analytics-highchart',
      'type' => 'line',
      'marginRight' => 130, // If we suppress legend, this gets set to 0 below
      'marginBottom' => 50,
    ),
    'title' => array(
      'text' => $metric_fullname
    ),
    'xAxis' => array(
      'categories' => array(), // Filled in below
      'title' => array(
        'text' => t('Time (UTC)'),
        'margin' => 0
      )
    ),
    'yAxis' => array(
      'title' => array('text' => $metric_fullname), // may be modified below
      'plotLines' => array(
        array(
          'value' => 0,
          'width' => 1,
          'color' => '#808080'
        )
      ),
      'min' => 0
    ),
    'legend' => array(
      'layout' => 'vertical',
      'align' => 'right',
      'verticalAlign' => 'top',
      'x' => -10,
      'y' => 100,
      'borderWidth' => 0
    ),
    'series' => array(), // filled in below
    'credits' => array('enabled' => FALSE),
    'plotOptions' => array(
      'line' => array(
        'marker' => array(
          'enabled' => FALSE
        )
      )
    )
  );

  if (substr($metric_name, -5) == '_time') {
    $chart_data['yAxis']['title']['text'] = t('Milliseconds');
  }

  // Step across X-axis labels if there are too many.
  $step = 1;
  if (is_array($time_points) && (count($time_points) > DEVCONNECT_ANALYTICS_MAX_XAXIS_LINES)) {
    $step = ceil(count($time_points) / DEVCONNECT_ANALYTICS_MAX_XAXIS_LINES);
    $chart_data['xAxis']['labels'] = array('step' => $step);
  }

  $app = $datapoints[$app_name];

  $last_time = 0;
  foreach ($time_points as $i => $time_point) {
    $x_caption = date($date_format, $time_point);
    // If granularity < day and date has changed, show date under time.
    if (substr($date_format, 0, 1) == 'H' && ($i % $step == 0) && ($last_time == 0 || date('d', $time_point) != date('d', $last_time))) {
      // PLEASE NOTE: Not sure, but I think inline styles are necessary here because they are being embedded in a Highchart
      $x_caption .= ' <div style="font-size:0.75em;font-style:italic;margin-top:-3px">' . date('F j', $time_point) . '</div>';
      $chart_data['xAxis']['labels']['useHTML'] = TRUE;
      $last_time = $time_point;
    }

    $chart_data['xAxis']['categories'][] = $x_caption;
  }

  // Read in all values for all environments.
  $data = array();
  foreach ($app as $env => $env_datapoints) {
    $data[$env] = $default_datapoints;
    foreach ($env_datapoints as $time_point => $value) {
      $data[$env][$time_point] = $value;
    }
  }

  if (count($data) == 1) {
    $chart_data['legend']['enabled'] = FALSE;
    $chart_data['chart']['marginRight'] = 0;
  }

  foreach ($data as $env => $time_points) {
    $chart_data['series'][] = array(
      'name' => $env,
      'data' => array_values($time_points),
    );
  }

  // Allow other modules to alter the chart settings.
  drupal_alter('devconnect_developer_apps_analytics_chart_data', $chart_data);

  $chart_json = json_encode($chart_data);
  $script = "jQuery(document).ready(function(){chart=new Highcharts.Chart($chart_json);});";
  drupal_add_js($script, array('type' => 'inline'));

  $query = array(
    'from' => $my_data['params']['start'],
    'to' => $my_data['params']['end'],
    'metric' => $metric_name,
    'unit' => $my_data['params']['unit'],
  );
  drupal_static(__FUNCTION__, $query);

  // TODO: FIXME eliminate inline style.
  return '<div id="analytics-highchart" style="min-width:400px; height: 300px; margin: 0 auto"></div>';

}

function _devconnect_developer_apps_analytics_get_datapoints($user, $app_name, $params = array()) {
  $dca = _devconnect_developer_apps_analytics_api_client();

  $metrics = _devconnect_developer_apps_analytics_metrics();
  $units = array('second', 'minute', 'hour', 'day', 'week', 'month', 'year');

  if (isset($_GET['metric']) && isset($metrics[$_GET['metric']])) {
    $metric = $_GET['metric'];
  }
  else if(isset($params['metric']) && isset($metrics[$params['metric']])) {
    $metric = $params['metric'];
  }
  else {
    $metric = 'total_response_time';
  }

  if (isset($_GET['to'])) {
    $end = $_GET['to'];
  }
  else if(isset($params['to'])) {
    $end = $params['to'];
  }
  else {
    $end = time();
  }

  if (isset($_GET['from']) && $_GET['from'] < $end) {
    $start = $_GET['from'];
  }
  else if(isset($params['from'])) {
    $start = $params['from'];
  }
  else {
    $start = strtotime('-24 hours', $end);
  }

  if (isset($_GET['unit']) && in_array($_GET['unit'], $units)) {
    $unit = $_GET['unit'];
  }
  else if(isset($params['unit'])) {
    $unit = $params['unit'];
  }
  else {
    $unit = 'hour';
  }
  $order = 'ASC';

  // 4G accepts UTC dates.
  $end = strtotime(gmdate('Y-m-d H:i:s', $end));
  $start = strtotime(gmdate('Y-m-d H:i:s', $start));

  $time_points = array();
  switch ($unit) {
    case 'second':
      $date_format = 'H:i:s';
      $current_time = $start;
      while ($current_time <= $end) {
        $time_points[] = $current_time;
        $current_time++;
      }
      break;
    case 'minute':
      $date_format = 'H:i';
      $current_time = $start - ($start % 60);
      while ($current_time <= $end) {
        $time_points[] = $current_time;
        $current_time += 60;
      }
      break;
    case 'hour':
      $date_format = 'H:00';
      $current_time = $start - ($start % 3600);
      while ($current_time <= $end) {
        $time_points[] = $current_time;
        $current_time += 3600;
      }
      break;
    case 'day':
      $date_format = 'j M';
      $current_time = $start - ($start % 86400);
      while ($current_time <= $end) {
        $time_points[] = $current_time;
        $current_time += 86400;
      }
      break;
    case 'week':
      $date_format = 'j M';
      $week_seconds = 86400 * 7;
      $current_time = $start - ($start % $week_seconds);
      while ($current_time <= $end) {
        $time_points[] = $current_time;
        $current_time += $week_seconds;
      }
      break;
    case 'month':
      $date_format = 'M Y';
      $current_time = mktime(0, 0, 0, date('m', $start), 1, date('Y', $start));
      while ($current_time <= $end) {
        $time_points[] = $current_time;
        $current_time = strtotime('+1 month', $current_time);
      }
      break;
    case 'year':
      $date_format = 'Y';
      $current_time = mktime(0, 0, 0, 1, 1, date('Y', $start));
      while ($current_time <= $end) {
        $time_points[] = $current_time;
        $current_time = strtotime('+1 year', $current_time);
      }
      break;
  }
  $default_datapoints = array();
  foreach ($time_points as $time_point) {
    $default_datapoints[$time_point] = 0;
  }

  // We will pass all the parameters that are required to fetch the analytics
  // data from the backend and that are required to render the data on the
  // graph as an array to drupal_alter() to be modified by other modules.
  $info['developer_id'] = devconnect_user_get_developer_id($user);
  $info['app_name'] = $app_name;
  $info['metric'] = $metric;
  $info['start'] = $start;
  $info['end'] = $end;
  $info['unit'] = $unit;
  $info['order'] = $order;
  $info['time_points'] = $time_points;
  $info['default_datapoints'] = $default_datapoints;
  $info['timezone'] = "UTC";

  // Pass the $info to drupal_alter to be modified by other modules.
  drupal_alter('devconnect_developer_app_analytics_datapoints_params', $info);

  $developer_email_or_company = $user->mail;
  // If the analytics are for a company app, we need to filter by company instead
  // of developer
  $is_company = FALSE;
  if (module_exists('apigee_company')) {
    $company = apigee_company_get_current_context_company();
    // If a user is in the company context then load the plans purchased by the company.
    if (!empty($company)) {
      $developer_email_or_company = $company->name;
      $is_company = TRUE;
    }
  }

  try {
    // We pass the altered version of the parameters to the call to get the
    // analytics data.
    $datapoints = $dca->getByAppName($developer_email_or_company, $app_name, $info['metric'], $info['start'], $info['end'], $info['unit'], $info['metric'], $info['order'], $is_company);
  } catch (Apigee\Exceptions\ParameterException $e) {
    watchdog('devconnect_developer_apps', $e->getMessage(), array(), WATCHDOG_ERROR);
    return FALSE;
  } catch (Exception $e) {
    watchdog('devconnect_developer_apps', $e->getMessage(), array(), WATCHDOG_ERROR);
    return FALSE;
  }

  // Allow modules to alter the datapoints.
  drupal_alter('devconnect_developer_app_analytics_datapoints', $app_name, $datapoints);

  // We return the altered version of the parameters so that it is reflected in
  // the graph and the downloads.
  return array(
    'date_format' => $date_format,
    'datapoints' => $datapoints,
    'default_datapoints' => $info['default_datapoints'],
    'time_points' => $info['time_points'],
    'params' => array(
      'metric' => array(
        'display_name' => $metrics[$metric],
        'name' => $metric
      ),
      'start' => $info['start'],
      'end' => $info['end'],
      'unit' => $info['unit'],
      'timezone' => $info['timezone'],
    ),
  );

}

function devconnect_developer_analytics_performance_download($user, $appId, $format) {

  $entities = entity_load('developer_app', array($appId), array('mail' => $user->mail));
  $entity = reset($entities);
  $app_name = $entity->name;

  $my_data = _devconnect_developer_apps_analytics_get_datapoints($user, $app_name);
  if ($my_data === FALSE) {
    return '<p>' . t('An error occurred while fetching your data.') . '</p>';
  }
  $datapoints = $my_data['datapoints'];
  $default_datapoints = $my_data['default_datapoints'];

  $app = $datapoints[$app_name];

  // Read in all values for all environments.
  $data = array();
  foreach ($app as $env => $env_datapoints) {
    $data[$env] = $default_datapoints;
    foreach ($env_datapoints as $time_point => $value) {
      $data[$env][$time_point] = $value;
    }
  }
  $filename_base = preg_replace('![^A-Za-z0-9_.-]!', '_', "$app_name-" . $my_data['params']['unit'] . '-' . date('Ymd-His', $my_data['params']['start']));
  if ($format == 'csv') {
    $content_type = 'text/comma-separated-values';
    $content = _devconnect_developer_apps_analytics_to_csv($my_data['params'], $data);
    $extension = 'csv';
  }
  else {
    $content_type = 'text/xml';
    $content = _devconnect_developer_apps_analytics_to_xml($my_data['params'], $data);
    $extension = 'xml';
  }
  header("Content-Type: $content_type");
  header("Content-Disposition: attachment; filename=\"$filename_base.$extension\"");
  header("Content-Length: " . strlen($content));
  print $content;

  module_invoke_all('exit');
  exit;
}


function _devconnect_developer_apps_analytics_to_xml($params, $data) {
  $document = '<?xml version="1.0" encoding="utf-8"?>'
    . '<analytics>'
    . '<params>'
    . '<metric>' . htmlspecialchars($params['metric']['display_name']) . '</metric>'
    . '<start>' . date('Y-m-d H:i:s', $params['start']) . " " . $params['timezone'] . '</start>'
    . '<end>' . date('Y-m-d H:i:s', $params['end']) . " " . $params['timezone'] . '</end>'
    . '<time-unit>' . htmlspecialchars($params['unit']) . '</time-unit>'
    . '</params>'
    . '<data>';
  //kpr($data); kpr($time_points); die();
  foreach ($data as $env => $series) {
    $document .= '<environment name="' . htmlspecialchars($env) . '">';
    foreach ($series as $time => $point) {
      $document .= '<point timestamp="' . date('Y-m-d H:i:s', $time) . '">' . htmlspecialchars($point) . '</point>';
    }
    $document .= '</environment>';
  }
  $document .= '</data></analytics>';
  return $document;
}


function _devconnect_developer_apps_analytics_to_csv($params, $data) {
  $csv = t('Metric') . ',' . $params['metric']['display_name'] . "\n"
    . t('Start') . ',' . date('Y-m-d H:i:s', $params['start']) . " " . $params['timezone'] . "\n"
    . t('End') . ',' . date('Y-m-d H:i:s', $params['end']) . " " . $params['timezone'] . "\n"
    . t('Time Unit') . ',' . $params['unit'] . "\n"
    . "\n\n";

  foreach ($data as $env => $series) {
    $csv .= t('Environment') . ',' . $env . "\n\n";
    foreach ($series as $time => $point) {
      $csv .= date('Y-m-d H:i:s', $time) . ',' . $point . "\n";
    }
    $csv .= "\n\n";
  }
  return $csv;
}


<?php

/**
 * @file
 * Admin interface for IP_Path_Access.
 */

define("PAGER_SIZE", 20);

/**
 * Menu callback. Display Allowed IP addresses / path pairs.
 *
 * Optional IP address to be passed on to drupal_get_form() for
 * use as the default value of the IP address form field.
 *
 * @param string $default_ip
 *   An IP.
 *
 * @return mixed
 *   Returns the form for the path / IP pairs.
 */
function ip_path_access($default_ip = '') {
  $rows = array();

  $header = array(
    array('data' => t('Title')),
    array('data' => t('Path Alias')),
    array('data' => t('Normalized Path'), 'field' => 'path', 'sort' => 'ASC'),
    array('data' => t('Allowed IP Address'), 'field' => 'ip', 'sort' => 'ASC'),
    array('data' => t('Custom Redirect Path')),
    array('data' => t('Operations')),
  );

  // Pull in the database entries for the page we are on, use a pager.
  $sql = 'SELECT * FROM {ip_path_access}';
  $query = db_select('ip_path_access', 'ip')
    ->fields('ip')
    ->extend('PagerDefault')
    ->extend('TableSort')
    ->limit(PAGER_SIZE)
    ->orderByHeader($header)
    ->execute();

  foreach ($query as $ip) {
    $rows[] = array(
      $ip->title,
      drupal_get_path_alias($ip->path),
      $ip->path,
      $ip->ip,
      $ip->custom_redirect,
      _ip_path_access_op($ip),
    );
  }

  $build['ip_path_access_form'] = drupal_get_form('ip_path_access_form', $default_ip);

  $build['ip_path_access_table'] = array(
    '#theme' => 'table',
    '#header' => $header,
    '#rows' => $rows,
    '#empty' => t('No allowed IP addresses available.'),
    '#weight' => 100,
  );

  $build['ip_path_access_form']['pager']
    = array(
      '#type' => 'markup',
      '#markup' => theme('pager', array('quantity', count($query))),
      '#weight' => 101,
    );

  return $build;
}


/**
 * List operations available for this ip/path row.
 *
 * @param string $ip
 *   A String with an IP object w/ unique ID to build the delete path.
 *
 * @return string
 *   Returns the link to a delete button for this row.
 */
function _ip_path_access_op($ip) {
  return l(t('delete'), "admin/config/people/ip_path_access/delete/$ip->iid");
}


/**
 * Implements hook_form().
 *
 * Define the form for Allowed Admin IP addresses / path pairs.
 *
 * @ingroup forms
 *
 * @see ip_path_access_form_validate()
 * @see ip_path_access_form_submit()
 */
function ip_path_access_form($form, $form_state, $default_ip) {
  $enabled = variable_get('ip_path_access_enabled', 0);
  $redirect_message = variable_get('ip_path_access_redirect_message', '');
  if (module_exists('og')) {
    $og_bypass_enabled = variable_get('ip_path_access_og_bypass_enabled', 0);
  }
  $form['help'] = array(
    '#type' => 'markup',
    '#markup' => t('
    <p>!ip_msg</p>
    <p>
    The IP Path Access module allows administrators to set specific valid Drupal
    pages to be accessed only by specified IP addresses, and no others, except
    for site administrators. By adding a Path/IP pair below in the interface
    that page will be blocked for all IPs other than the IP specified.<br />
    This module has permissions that allows logged-in user-roles to bypass IP
    restrictions.<br />
    In essence the module is a per-page, block all IPs, except the IPs defined
    for the page to allow access (and defined user-roles by permissions.)
    </p>
    <p>Below: settings.php enty that can be used to enable/disable.</p>
    "$conf[\'ip_path_access_enabled\'] = 0;" deactivates the module.<br />
    <!--"$conf[\'ip_path_access_testing\'] = 1;" to set the module for local testing.<br /><br />-->
    ',
    array('!ip_msg' => _ip_path_access_user_current_ip_msg())),
  );
  $form['ip_enable_settings_title'] = array(
    '#type' => 'markup',
    '#markup' => '<br /><h3>' . t('Enable / Disable Options') . '</h3>',
  );
  $form['enabled'] = array(
    '#title' => t('Enable IP access by path'),
    '#type' => 'checkbox',
    '#default_value' => $enabled,
    '#description' => t('When enabled, path/IP pairs below will be allowed, all other blocked.'),
  );
  if (module_exists('og')) {
    $form['og_bypass'] = array(
      '#title' => t('Enable Organic Groups users to bypass IP access'),
      '#type' => 'checkbox',
      '#default_value' => $og_bypass_enabled,
      '#description' => t('When enabled Organic Group members bypass this modules IP access rules.'),
    );
  }
  $form['ip_settings_title'] = array(
    '#type' => 'markup',
    '#markup' => '<br /><h3>' . t('Restrict Path on IP') . '</h3>',
  );
  $form['ip_path_rule'] = array(
    '#title' => t('Title'),
    '#type' => 'textfield',
    '#size' => 60,
    '#maxlength' => 128,
  );
  $form['ip'] = array(
    '#title' => t('IP address'),
    '#type' => 'textfield',
    '#size' => 48,
    '#maxlength' => 48,
    '#description' => t('IP address. Format of:<br />Single IP number: 1.2.3.4<br />IP range: 1.2.3.4 - 5.6.7.8'),
  );
  $form['path'] = array(
    '#title' => t('Path'),
    '#type' => 'textfield',
    '#size' => 48,
    '#maxlength' => 255,
    '#description' => t('Enter a valid path.'),
  );
  $form['custom_redirect'] = array(
    '#title' => t('Custom redirect'),
    '#type' => 'textfield',
    '#size' => 48,
    '#maxlength' => 255,
    '#description' => t('Enter a valid path to redirect the user when denied access.'),
  );
  $form['redirect_message'] = array(
    '#title' => t('Redirect message'),
    '#type' => 'textarea',
    '#description' => t('Enter a warning message to display when the user is redirected to the custom redirect path. Leave this text area blank to not display a warning message to the user.'),
    '#rows' => 3,
    '#default_value' => $redirect_message,
  );
  $form['actions'] = array('#type' => 'actions');
  $form['actions']['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Add Path and/or Update Options'),
  );
  $form['#submit'][] = 'ip_path_access_form_submit';
  $form['#validate'][] = 'ip_path_access_form_validate';
  return $form;
}


/**
 * Show message of current IP.
 *
 * @return string
 *   Returns markup of message of current IP.
 */
function _ip_path_access_user_current_ip_msg() {
  $curr_ip = ip_address();
  $output = t('Your current IP is @ip.', array('@ip' => $curr_ip)) . '<br />';
  return $output;
}


/**
 * Form validate function for IP/path pair.
 */
function ip_path_access_form_validate($form, &$form_state) {
  $ip = trim($form_state['values']['ip']);
  $path = trim($form_state['values']['path']);
  $custom_redirect = trim($form_state['values']['custom_redirect']);

  // Check if saving form to update options, not saving a new path/ip.
  if ($ip == "" && $path == "") {
    drupal_set_message(t("Updating enable/disable options only (no IP / Path provided)."));
  }
  // Else check if IP and path are valid.
  else {
    // Is this a valid IP?
    if (!_ip_path_access_validate_ip_range($ip)) {
      form_set_error('ip', t('This is not a valid IP or range.'));
    }
    if (strpos ($path, '*') == FALSE) {
      if (!drupal_valid_path (drupal_get_normal_path ($path), TRUE)) {
        form_set_error ('path', t ('This path is not valid.'));
      }
    }
    if (
      !empty($custom_redirect) &&
        (
        !drupal_valid_path(drupal_get_normal_path($custom_redirect), TRUE) ||
        url_is_external($custom_redirect)
        )
      ) {
      form_set_error('custom_redirect', t('This redirect path is not valid.'));
    }
    // Check if we have a match in the database alread, do not accept.
    $existing_ip_path_match = db_query(
      "SELECT ip, path FROM {ip_path_access} WHERE ip = :ip AND path = :path",
      array(':ip' => $ip, ':path' => drupal_get_normal_path($path))
      )->fetchAll();
    if (count($existing_ip_path_match) > 0) {
      form_set_error('path', t('This entry already exists in the database.'));
    }
    if($path == $custom_redirect){
      form_set_error('path', t('Both Path and Custom redirect path cannot be the same.'));
      form_set_error('custom_redirect', '');
    }
  }
}


/**
 * Form submit function for IP / path pair.
 *
 * Submit and store ip/path pair into database.
 */
function ip_path_access_form_submit($form, &$form_state) {
  variable_set('ip_path_access_enabled', $form_state['values']['enabled']);
  variable_set('ip_path_access_redirect_message', $form_state['values']['redirect_message']);
  if (module_exists('og')) {
    variable_set('ip_path_access_og_bypass_enabled', $form_state['values']['og_bypass']);
  }

  $title = trim($form_state['values']['ip_path_rule']);
  $ip = trim($form_state['values']['ip']);
  $path = trim($form_state['values']['path']);
  $custom_redirect = trim($form_state['values']['custom_redirect']);

  // Store the submitted IP with path.
  if ($ip != '') {
    $ip = str_replace(' ', '', $ip);
    db_insert('ip_path_access')
      ->fields(array('title' => $title, 'ip' => $ip, 'path' => drupal_get_normal_path($path), 'custom_redirect' => drupal_get_normal_path($custom_redirect)))
      ->execute();
    if(!empty($custom_redirect)){
      drupal_set_message(
        t(
          'The Path [@path], IP address [@ip] and Custom redirect [@custom_redirect] has been added.',
          array('@path' => $path, '@ip' => $ip, '@custom_redirect' => $custom_redirect)
        )
      );
    }else{
      drupal_set_message(
        t(
          'The Path [@path] and IP address [@ip] pair has been added.',
          array('@path' => $path, '@ip' => $ip)
        )
      );
    }
  }

  $form_state['redirect'] = 'admin/config/people/ip_path_access';
}


/**
 * IP deletion confirm page.
 *
 * @see ip_path_access_delete_submit()
 */
function ip_path_access_delete($form, &$form_state, $iid) {
  $ip = db_query("SELECT * FROM {ip_path_access} WHERE iid = :iid", array(':iid' => $iid))->fetchAssoc();
  $form['iid'] = array(
    '#type' => 'value',
    '#value' => $iid,
  );
  $form['ip'] = array(
    '#type' => 'hidden',
    '#value' => $ip['ip'],
  );
  return confirm_form($form, t('Are you sure you want to delete @ip / at @path?', array('@ip' => $ip['ip'], '@path' => $ip['path'])), 'admin/config/people/ip_path_access', t('This action cannot be undone.'), t('Delete'), t('Cancel'));
}


/**
 * Process ip_path_access_delete form submissions.
 */
function ip_path_access_delete_submit($form, &$form_state) {
  db_delete('ip_path_access')
      ->condition('iid', $form_state['values']['iid'])
      ->execute();
  watchdog('ip_path_access', 'Deleted allowed admin IP @ip', array('@ip' => $form_state['values']['ip']));
  drupal_set_message(t('The IP address @ip was deleted.', array('@ip' => $form_state['values']['ip'])));
  $form_state['redirect'] = 'admin/config/people/ip_path_access';
}

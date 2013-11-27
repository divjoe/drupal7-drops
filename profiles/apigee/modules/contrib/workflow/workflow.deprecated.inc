<?php
/**
 * @file
 * Contains contains per-class functions, that are deprecated.
 * Usage: The new code can be tested, by removing this file-include from workflow.module.
 */

/**
 * Deprecated functions related to table {workflows}.
 * These are replaced by methods of class Workflow.
 */

/**
 * Get all workflows.
 * @deprecated: workflow_get_workflows() --> Workflow::getWorkflows($wid)
 */
function workflow_get_workflows() {
  return Workflow::getWorkflows();
}

/**
 * Get a specific workflow, wid is a unique ID.
 * @deprecated: workflow_get_workflows_by_wid() --> Workflow::load($wid)
 */
function workflow_get_workflows_by_wid($wid, $reset = FALSE) {
  return Workflow::load($wid, $reset);
}

/**
 * Get a specific workflow, name is a unique ID.
 * @deprecated: workflow_get_workflows_by_name() --> Workflow::getWorkflowByName($name)
 */
function workflow_get_workflows_by_name($name, $unserialize_options = FALSE) {
  return Workflow::getWorkflowByName($name, $unserialize_options);
}

/**
 * Return the ID of the creation state for this workflow.
 * @deprecated: workflow_get_creation_state_by_wid($wid) --> $workflow->getCreationSid().
 *
 * @param mixed $wid
 *   The ID of the workflow.
 */
function workflow_get_creation_state_by_wid($wid) {
  $sid = 0;
  if ($workflow = Workflow::load($wid)) {
    $sid = $workflow->getCreationSid();
  }
  return $sid;
}

/**
 * Return the ID of the creation state given a content type.
 *
 * @param string $type
 *   The type of the content.
 */
function workflow_get_creation_state_by_type($type) {
  $sid = FALSE;
  if ($workflow = workflow_get_workflows_by_type($type)) {
    $sid = $workflow->getCreationSid();
  }
  return $sid;
}

/**
 * Given information, update or insert a new workflow. Returns data by ref. (like node_save).
 *
 * @deprecated: workflow_update_workflows() --> Workflow->save()
 */
function workflow_update_workflows(&$data, $create_creation_state = TRUE) {
  $data = (object) $data;
  if (isset($data->tab_roles) && is_array($data->tab_roles)) {
    $data->tab_roles = implode(',', $data->tab_roles);
  }

  if (isset($data->wid) && Workflow::load($data->wid)) {
    drupal_write_record('workflows', $data, 'wid');
  }
  else {
    drupal_write_record('workflows', $data);
    if ($create_creation_state) {
      $state_data = array(
        'wid' => $data->wid,
        'state' => t('(creation)'),
        'sysid' => WORKFLOW_CREATION,
        'weight' => WORKFLOW_CREATION_DEFAULT_WEIGHT,
        );

      workflow_update_workflow_states($state_data);
      // @TODO consider adding state data to return here as part of workflow data structure.
      // That way we could past structs and transitions around as a data object as a whole.
      // Might make clone easier, but it might be a little hefty for our needs?
    }
  }
}

/**
 * Given a wid, delete the workflow and its data.
 *
 * @deprecated: workflow_delete_workflows_by_wid() --> Workflow::delete().
 */
function workflow_delete_workflows_by_wid($wid) {
  $workflow = Workflow::load($wid);
  $workflow->delete();
}

/**
 * Deprecated functions related to table {workflow_states}.
 * These are replaced by methods of class WorkflowState.
 */

/**
 * Get all active states in the system.
 *
 * @return array
 *  a keyed array $id => $name, of all active states.
 * @deprecated: workflow_get_workflow_states_all() --> Workflow::getOptions()
 */
function workflow_get_workflow_states_all() {
  $options = array();

  // Get all states, only where active.
  foreach (Workflow::getWorkflows() as $workflow) {
    $options += $workflow->getOptions($grouped = FALSE);
  }

  return $options;
}

/**
 * Menu access control callback. Determine access to Workflow tab.
 *
 * @deprecated workflow_node_tab_access() --> workflow_tab_access().
 */
function workflow_node_tab_access($node = NULL) {
  if ($node == NULL) {
    return FALSE;
  } 
  return workflow_tab_access('node', $node);
}

/**
 * Get the states current user can move to for a given node.
 * @deprecated workflow_field_choices() --> WorkflowState->getOptions()
 *
 * @param object $node
 *  The node to check.
 * @param bool $force
 *  A switch to enable access to all states (e.g. for Rules)
 * @param State $state
 *  The predetermined state object (v7.x-1.3: new parameter for Workflow Field.)
 *
 * @return array
 *  Array of transitions.
 */
function workflow_field_choices($node, $force = FALSE, $state = NULL) {
  $choices = array();

  if (!$node) {
    // If no node is given, no result. (e.g., on a Field settings page.)
    return $choices;
  }

  if ($state) {
    // This is used in Field API. A state object is already passed in.
  }
  else {
    // This is used in Node API.
    $workflow = workflow_get_workflow_type_map_by_type($node->type);
    $current_sid = workflow_node_current_state($node);
    $state = WorkflowState::load($current_sid);
  }
  return $state->getOptions('node', $node, $force);
}

/**
 * Determine if the Workflow Form must be shown.
 * If not, a formatter must be shown, since there are no valid options.
 *
 * @deprecated workflow_show_form() --> WorkflowState->showWidget()
 *
 * @param mixed $sid
 *   the current state ID.
 * @param Workflow $workflow
 *   the workflow object (might be derived from $sid).
 * @param array $options
 *   an array with $id => $label options, as determined in WorkflowState->getOptions().
 *
 * @return bool $show_widget
 *   TRUE = a form must be shown; FALSE = no form, a formatter must be shown instead.
 */
function workflow_show_form($sid, $workflow, array $options) {
  $state = WorkflowState::load($sid);
  return !$state->showWidget($options);
}

/**
 * Validate target state and either execute a transition immediately or schedule
 * a transition to be executed later by cron.
 *
 * @deprecated: workflow_transition --> WorkflowDefaultWidget::submit()
 *
 * @param object $entity
 * @param string $new_sid
 *   An integer; the target state ID.
 * @param bool $force
 *   Allows bypassing permissions, primarily for Rules.
 * @param array $field
 *   The field structure for the operation.
 */
function workflow_transition($entity, $new_sid, $force = FALSE, $field = array()) {
  $entity_type = 'node'; // Entity support is in workflow_transition --> WorkflowDefaultWidget::submit()
  // @todo: do not use widget:submit directly, use workflow_entity_save instead.
  $widget = new WorkflowDefaultWidget($field, $instance = array(), $entity_type, $entity);
  $form = array();
  $form_state = array();
  $items = array();
  $items[0]['workflow'] = (array) $entity;
  $items[0]['workflow']['workflow_options'] = $new_sid;
  $widget->submit($form, $form_state, $items, $force);
}

/**
 * Get all states in the system by content type.
 */
function workflow_get_workflow_states_by_type($type) {
  $query = "SELECT ws.sid, ws.wid, ws.state, ws.weight, ws.sysid "
    . "FROM {workflow_type_map} wtm "
    . "INNER JOIN {workflow_states} ws ON ws.wid = wtm.wid "
    . "WHERE wtm.type = :type AND ws.status = 1 "
    . "ORDER BY ws.weight, ws.sid "
  ;
  $query_array = array(':type' => $type);
  $results = db_query($query, $query_array);
  return $results->fetchAll();
}

/**
 * Get all states in the system, with options to filter, only where a workflow exists.
 *
 * @deprecated: workflow_get_workflow_states() --> WorkflowState::getStates()
 * @deprecated: workflow_get_workflow_states_by_wid() --> WorkflowState::getStates()
 */
function workflow_get_workflow_states($options = array()) {
  // Build the basic query.
  $query = db_select('workflow_states', 'ws');
  $query->leftJoin('workflows', 'w', 'w.wid = ws.wid');
  $query->fields('ws');
  $query->addField('w', 'wid');
  $query->addField('w', 'name');

  // Spin through the options and add conditions.
  foreach ($options as $column => $value) {
    $query->condition('ws.' . $column, $value);
  }

  // Set the sorting order.
  $query->orderBy('ws.wid');
  $query->orderBy('ws.weight');

  // Just for grins, add a tag that might result in modifications.
  $query->addTag('workflow_states');

  // Give them the answer.
  return $query->execute()->fetchAllAssoc('sid');
}

/**
 * Get all states in the system, with options to filter, only where a workflow exists.
 *
 * @deprecated: workflow_get_workflow_states_by_wid() --> Workflow->getOptions()
 */
function workflow_get_workflow_states_by_wid($wid, $options = array()) {
  $options['wid'] = $wid;
  return workflow_get_workflow_states($options);
}

/**
 * Given a sid, return a workflow. Sids are a unique id.
 * @deprecated: workflow_get_workflow_by_sid --> WorkflowState::load()
 */
function workflow_get_workflow_by_sid($sid) {
  return db_query("SELECT w.wid, w.name, w.tab_roles, w.options FROM {workflow_states} s
    INNER JOIN {workflows} w ON w.wid=s.wid WHERE sid = :sid ",
    array(':sid' => $sid))->fetchObject();
}

/**
 * Given a sid, return a state. Sids are a unique id.
 * @deprecated: workflow_get_workflow_states_by_sid($sid) --> WorkflowState::load($sid)
 */
function workflow_get_workflow_states_by_sid($sid, $options = array()) {
  static $sids = array();
  if (!isset($sids[$sid])) {
    $states = workflow_get_workflow_states(array('sid' => $sid));
    $sids[$sid] = reset($states);
  }
  return $sids[$sid];
}

/**
 * Given a sid, return all other states in that workflow.
 *
 * @deprecated: replaced by WorkflowState::getStates($sid)
 */
function workflow_get_other_states_by_sid($sid) {
  $query = "SELECT sid, state "
    . "FROM {workflow_states} "
    . "WHERE wid = (SELECT wid FROM {workflow_states} WHERE sid = :sid AND status = 1 AND sysid = 0) "
  ;
  return db_query($query, array(':sid' => $sid))->fetchAllKeyed();
}

/**
 * Given a wid and state, return a state. Wids / states are a unique id.
 */
function workflow_get_workflow_states_by_wid_state($wid, $state) {
  $options = array(
    'state' => $state,
    'wid' => $wid,
  );
  return workflow_get_workflow_states($options);
}

/**
 * Given a sid, delete the state and all associated data.
 * @deprecated: workflow_delete_workflow_states_by_sid($sid, $new_sid, $true_delete) --> WorkflowState->delete()
 */
function workflow_delete_workflow_states_by_sid($sid, $new_sid = FALSE, $true_delete = FALSE) {
  if ($state = WorkflowState::load($sid)) {
    $state->delete($new_sid, $true_delete);
  }
}

/**
 * Save (update/insert) a Workflow State into table {workflow_states}.
 * @deprecated: workflow_update_workflow_states() --> WorkflowState->save()
 */
function workflow_update_workflow_states(&$data) {
  $data = (object) $data;
  if (!isset($data->sysid)) {
    $data->sysid = 0;
  }
  if (!isset($data->status)) {
    $data->status = 1;
  }
  if (isset($data->sid) && WorkflowState::load($data->sid)) {
    drupal_write_record('workflow_states', $data, 'sid');
  }
  else {
    drupal_write_record('workflow_states', $data);
  }
}

/**
 * Deprecated functions related to table {workflow_scheduled_transition}.
 * These are replaced by methods of class WorkflowScheduledTransition.
 */

/**
 * Given a node, get all scheduled transitions for it.
 * @deprecated: workflow_get_workflow_scheduled_transition_by_nid() --> WorkflowScheduledTransition::load()
 */
function workflow_get_workflow_scheduled_transition_by_nid($nid) {
  return WorkflowScheduledTransition::load('node', $nid);
}

/**
 * Given a timeframe, get all scheduled transitions.
 * @deprecated: workflow_get_workflow_scheduled_transition_by_between() --> WorkflowScheduledTransition::loadBetween()
 */
function workflow_get_workflow_scheduled_transition_by_between($start = 0, $end = REQUEST_TIME) {
  return WorkflowScheduledTransition::loadBetween($start, $end);
}

/**
 * Insert a new scheduled transition. Only one transition at a time (for now).
 * @deprecated: workflow_insert_workflow_scheduled_transition() --> WorkflowScheduledTransition::save()
 */
function workflow_insert_workflow_scheduled_transition($data) {
  $data = (object) $data;
  workflow_delete_workflow_scheduled_transition_by_nid($data->nid);
  drupal_write_record('workflow_scheduled_transition', $data);
}

/**
 * Given a node, delete transitions for it.
 * @deprecated: workflow_delete_workflow_scheduled_transition_by_nid() --> WorkflowScheduledTransition::delete()
 *        It is still used in workflow_execute_transition().
 */
function workflow_delete_workflow_scheduled_transition_by_nid($nid) {
  return WorkflowScheduledTransition::deleteById('node', $nid);
}

/**
 * Deprecated functions related to table {workflow_node_history}.
 */

/**
 * Get all recorded history for a node id.
 *
 * Since this may return a lot of data, a limit is included to allow for only one result.
 */
function workflow_get_workflow_node_history_by_nid($nid, $limit = NULL) {
  return WorkflowTransition::load('node', $nid, $field_name= '', $limit);
}

/**
 * Given data, insert a new history. Always insert.
 *
 * @deprecated: workflow_insert_workflow_node_history() --> WorkflowTransition::save()
 */
function workflow_insert_workflow_node_history($data) {
  $data = (object) $data;
  if (isset($data->hid)) {
    unset($data->hid);
  }

  // Check for no transition.
  if ($data->old_sid == $data->sid) {
    // Make sure we haven't already inserted history for this update.
    $last_history = workflow_get_workflow_node_history_by_nid($data->nid, 1);
    if (isset($last_history) && $last_history->stamp == REQUEST_TIME) {
      return;
    }
  }
  drupal_write_record('workflow_node_history', $data);
}
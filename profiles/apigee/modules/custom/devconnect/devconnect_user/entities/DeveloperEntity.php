<?php

namespace Drupal\devconnect_user;
use \Drupal\devconnect\ArrayEntity;

class DeveloperEntity extends ArrayEntity {
  /**
   * @var array
   */
  public $apps;
  /**
   * @var string
   * This is actually the unique-key (within the org) for the Developer
   */
  public $email;
  /**
   * @var string
   * Read-only alternate unique ID. Useful when querying developer analytics.
   */
  public $developerId;
  /**
   * @var string
   */
  public $firstName;
  /**
   * @var string
   */
  public $lastName;
  /**
   * @var string
   */
  public $userName;
  /**
   * @var string
   * Read-only
   */
  public $organizationName;
  /**
   * @var string
   * Should be either 'active' or 'inactive'.
   */
  public $status;
  /**
   * @var array
   */
  public $attributes;
  /**
   * @var int
   * Read-only
   */
  public $createdAt;
  /**
   * @var string
   * Read-only
   */
  public $createdBy;
  /**
   * @var int
   * Read-only
   */
  public $modifiedAt;
  /**
   * @var string
   * Read-only
   */
  public $modifiedBy;
  
  public $debugData;
}

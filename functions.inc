<?php
/**
*   glFusion API functions for the Banner plugin
*
*   @author     Lee Garner <lee@leegarner.com>
*   @copyright  Copyright (c) 2009-2012 Lee Garner <lee@leegarner.com>
*   @package    banner
*   @version    0.1.7
*   @license    http://opensource.org/licenses/gpl-2.0.php 
*               GNU Public License v2 or later
*   @filesource
*/

if (!defined ('GVERSION')) {
    die ('This file can not be used on its own.');
}

/** Global configuration array, in case this is included within a function
*   @global array
*/
global $_CONF_BANR;

// Define banner types.  This is done very early because they're used
// in the language files 
define ('BANR_TYPE_LOCAL',  0);    // Locally-uploaded image
define ('BANR_TYPE_REMOTE', 1);    // Remote image url
define ('BANR_TYPE_SCRIPT', 2);    // Script, e.g. Google Adsense
define ('BANR_PI_PATH', $_CONF['path'] . 'plugins/banner');

$langfile = $_CONF['language'] . '.php';
if (!file_exists(BANR_PI_PATH . '/language/' . $langfile)) {
    $langfile = 'english.php';
}
/** Import the plugin's language file */
require_once BANR_PI_PATH . '/language/' . $langfile;

//  Check and see if we need to load the plugin configuration
if (!isset($_CONF_BANR['templatevars'])) {
    /** Import the configuration class, if needed */
    require_once $_CONF['path_system'] . 'classes/config.class.php';

    $ban_config = config::get_instance();
    $_CONF_BANR = $ban_config->get_config('banner');
}

/** Import static config values */
require_once BANR_PI_PATH . '/banner.php';
if (file_exists(BANR_PI_PATH . '/config.php')) {
    /** Import local configuration overrides, if any */
    include_once BANR_PI_PATH . '/config.php'; 
}

define ('BANR_ADMIN_URL', 
        $_CONF['site_admin_url'] . '/plugins/'.$_CONF_BANR['pi_name']);
define ('BANR_URL', 
        $_CONF['site_url'] . '/'.$_CONF_BANR['pi_name']);
define ('BANR_CLASSPATH', BANR_PI_PATH . '/classes');

/** Import Banner class */
function USES_banner_class_banner()
{   require_once BANR_CLASSPATH . '/banner.class.php';  }

/** Import Banner List class */
function USES_banner_class_bannerlist()
{   require_once BANR_CLASSPATH . '/bannerlist.class.php';  }

/** Import Banner Campaign class */
function USES_banner_class_campaign()
{   require_once BANR_CLASSPATH . '/campaign.class.php';    }

/** Import Campaign List class */
function USES_banner_class_campaignlist()
{   require_once BANR_CLASSPATH . '/campaignlist.class.php';    }

/** Import Category Class */
function USES_banner_class_category()
{   require_once BANR_CLASSPATH . '/category.class.php';    }

/** Import Image class */
function USES_banner_class_image()
{   require_once BANR_CLASSPATH . '/image.class.php';   }

/** Import Banner User Functions */
function USES_banner_userfunctions()
{   require_once BANR_PI_PATH . '/userfunctions.php';    }

/** Import plugin-specific functions */
function USES_banner_functions()
{   require_once BANR_PI_PATH . '/banner_functions.php';    }

/** Import installation defaults, typically for upgrades */
function USES_banner_install_defaults()
{   require_once BANR_PI_PATH . '/install_defaults.php';    }

/** Import upgrade functions */
function USES_banner_upgrade()
{   require_once BANR_PI_PATH . '/upgrade.inc.php';  }


/**
*   Create the centerblock, if needed
*   @param  integer $where  Which area is being displayed now
*   @param  integer $page   Page number
*   @param  string  $topic  Topic ID, or empty string
*   @return string          HTML for centerblock
*/
function plugin_centerblock_banner($where=1,$page=1,$topic ='')
{
    global $_CONF_BANR;

    // Flag whether we're on the homepage or not
    $on_homepage = ($page == 1 && $topic == '') ? true : false;

    // If centerblock not enabled, or just for homepage and
    // we're not on the homepage, just return
    if ($_CONF_BANR['cb_enable'] == 0 ||
            ($_CONF_BANR['cb_home'] == 1 && !$on_homepage) ) {
        return '';
    }

    USES_banner_functions();
    $banner = BANNER_centerblock($where, $page, $topic);
    if ($_CONF_BANR['cb_replhome'] == 1 && $on_homepage) {
        // Replacing the homepage, and we're currently on the homepage
        echo COM_siteHeader() . $banner . COM_siteFooter(true);
        exit;
    } else {
        // Return only the banner for a normal centerblock
        return $banner;
    }
}



/**
* Implements the [banner:] [randombanner:] autotag.
*
* @param    string  $op         operation to perform
* @param    string  $content    item (e.g. story text), including the autotag
* @param    array   $autotag    parameters used in the autotag
* @param    mixed               tag names (for $op='tagname') or formatted content
*/
function plugin_autotags_banner($op, $content = '', $autotag = '')
{
    global $_CONF, $_TABLES, $LANG_DIRECTION;

    if ($op == 'tagname' ) {
        return array('banner','randombanner','bannercategory');
    } else if ($op == 'parse') {
        $banner = '';

        switch( $autotag['tag'] ) {
        case 'banner':
            // Get a specific banner
            USES_banner_class_banner();
            $bid = COM_sanitizeID($autotag['parm1'], false);
            $B = new Banner($bid);
            $banner = $B->BuildBanner($title);
            $B->updateImpressions();
            break;

        case 'randombanner':
            // Get a random banner, optionally from a specific category
            USES_banner_class_banner();
            $cid = COM_applyFilter($autotag['parm1']);
            $options = array('limit' => 1);
            if (!empty($cid)) {
                $options['cid']= $cid;
            }
            $bids = Banner::GetBanner($options);
            $B = new Banner($bids[0]);
            $banner = $B->BuildBanner($title);
            $B->updateImpressions();
            break;

        case 'bannercategory' :
            USES_banner_class_banner();
            $cid = COM_applyFilter($autotag['parm1']);
            if (!empty($cid)) {
                // if there's a cid, join that table
                $cid_sql = "LEFT JOIN {$_TABLES['bannercategories']} AS c 
                    ON b.cid=c.cid 
                    WHERE c.cid='{$cid}' " .
                        COM_getPermSQL('AND', 0, 2, 'c') .
                         ' AND ';
            } else {
                $cid_sql = " WHERE ";   
            }
            $result = DB_query("SELECT b.bid
                    FROM {$_TABLES['banner']} AS b 
                    $cid_sql
                    (b.publishstart IS NULL OR b.publishstart < NOW()) 
                    AND (b.publishend IS NULL OR b.publishend > NOW()) 
                    ORDER BY RAND()");
            $T = new Template(BANR_PI_PATH . '/templates');
            $T->set_file('block', 'bannercategory.thtml');
            $T->set_block('block', 'bannerRow', 'BRow');
            $B = new Banner();
            while ($A = DB_fetchArray($result, false)) {
                $B->Read($A['bid']);
                $T->set_var('banner', $B->BuildBanner());
                $T->parse('BRow', 'bannerRow', true);
            }
            $T->parse('output','block');
            $banner = $T->finish($T->get_var('output'));
            break;

        }
        
        $content = str_replace($autotag['tagstr'], $banner, $content);
        return $content;
    }
}


/**
* Called by the plugin Editor to display the current plugin code version
* This may be different than the version installed and registered currently.
* If newer then you may want to run the update
*
* @return   string  version number
*/
function plugin_chkVersion_banner ()
{
    global $_CONF_BANR;
    return $_CONF_BANR['pi_version'];
}


/**
*   Shows the statistics for the banner plugin on stats.php.
*   If $showsitestats is 1 then we are to only print the overall stats in the
*   'site statistics box' otherwise we show the detailed stats
*
*   @param    int     $showsitestats  Flag to let us know which stats to get
*   @param    string                  HTML for the stats section
*/
/*function plugin_showstats_banner ($showsitestats)
{
    global $_CONF, $_TABLES, $LANG_BANNER;

    USES_lib_admin();
    USES_banner_class_banner();

    $retval = '';

    $result = DB_query ("SELECT bid
        FROM {$_TABLES['banner']} 
        WHERE (hits > 0) 
        AND (publishstart IS NULL OR publishstart < NOW()) 
        AND (publishend IS NULL OR publishend > NOW())" . 
        COM_getPermSQL ('AND') . 
        " ORDER BY hits DESC LIMIT 10");
    $nrows  = DB_numRows ($result);
    if ($nrows > 0) {
        $header_arr = array(
            array('text'         => $LANG_BANNER['title'],
                  'field'        => 'title',
                  'header_class' => 'stats-header-title'
            ),
            array('text'         => $LANG_BANNER['banner'],
                  'field'        => 'banner',
                  'header_class' => 'stats-header-title'
            ),
            array('text'         => $LANG_BANNER['hits'],
                  'field'        => 'hits',
                  'header_class' => 'stats-header-count',
                  'field_class'  => 'stats-list-count'
            )
        );
        $data_arr = array();
        $text_arr = array('has_menu' => false,
                          'title'    => $LANG_BANNER['stats_headline'],
        );
        $B = new Banner();
        for ($i = 0; $i < $nrows; $i++) {
            list($bid) = DB_fetchArray ($result);
            $B->Read($bid);
            $data_arr[] = array('title' => $B->title,
                            'banner' => $B->BuildBanner('', 200, 0, false),
                            'hits' => COM_numberFormat($B->hits),
                          );
        }
        $retval .= ADMIN_simpleList ('', $header_arr, $text_arr, $data_arr);
    } else {
        $retval .= COM_startBlock ($LANG_BANNER['stats_headline']);
        $retval .= $LANG_BANNER['stats_no_hits'];
        $retval .= COM_endBlock ();
    }

    return $retval;
}*/


/**
*   New stats plugin API function for proper integration with the site stats
*
*   @return array   item text, item count
*/
function plugin_statssummary_banner ()
{
    global $LANG_BANNER_STATS, $_TABLES;

    $result = DB_query("SELECT COUNT(*) AS count, SUM(hits) AS clicks 
        FROM {$_TABLES['banner']} 
        WHERE (publishstart IS NULL OR publishstart < NOW()) 
        AND (publishend IS NULL OR publishend > NOW())" . 
        COM_getPermSQL ('AND'));
    $A = DB_fetchArray ($result);

    $item_count = COM_NumberFormat($A['count'])
                . ' (' . COM_NumberFormat((int)$A['clicks']) . ')';

    return array ($LANG_BANNER_STATS['banner'], $item_count);
}


/**
*   This will put an option for banner in the command and control block on
*   moderation.php
*
*   @return mixed   array(title, url, icon), or boolean false when not allowed
*/
function plugin_cclabel_banner()
{
    global $LANG_BANNER;

    if (SEC_hasRights ('banner.edit')) {
        return array ($LANG_BANNER[14],
                      BANR_ADMIN_URL . '/index.php',
                      plugin_geticon_banner ());
    }

    return false;
}


/**
* returns the administrative option for this plugin
*
* @return   mixed   array(title, url, num. banner), or void when not allowed
*/
function plugin_getadminoption_banner()
{
    global $_TABLES, $LANG_BANNER;

    if (SEC_hasRights ('banner.edit,banner.delete', 'OR')) {
        $total_banner = DB_getItem ($_TABLES['banner'], 'COUNT(*)',
                                   COM_getPermSql (''));

        return array ($LANG_BANNER[14],
                      BANR_ADMIN_URL . '/index.php',
                      $total_banner);
    }
}


/**
*   A user is about to be deleted. Update ownership of any banners owned
*   by that user or delete them.
*
*   @param  integer $uid    User id of deleted user
*/
function plugin_user_delete_banner ($uid)
{
    global $_TABLES, $_CONF_BANR;

    if ($_CONF_BANR['delete_banner'] == 1) {
        // delete the banner
        DB_delete($_TABLES['banner'], 'owner_id', $uid);
    } else {
        // assign ownership to a user from the Banner Admin group
        $rootgroup = DB_getItem($_TABLES['groups'], 'grp_id',
                                 "grp_name = 'Root'");
        $result = DB_query("SELECT DISTINCT ug_uid 
                FROM {$_TABLES['group_assignments']} 
                WHERE ug_main_grp_id = $rootgroup 
                ORDER BY ug_uid 
                LIMIT 1");
        $A = DB_fetchArray ($result);
        $rootuser = (int)$A['ug_uid'];
        if ($rootuser > 0) {
            DB_query("UPDATE {$_TABLES['banner']} 
                SET owner_id = '$rootuser'
                WHERE owner_id = $uid");
        }
    }
}


/**
*   Support merging local accounts into remote accounts
*
*   @param  integer $origUID    Original (remote) user ID
*   @param  integer $destUID    Merged (local) user ID
*/
function plugin_user_move_banner($origUID, $destUID)
{
    global $_TABLES;

    $origUID = (int)$origUID;
    $destUID = (int)$destUID;

    // Tables containing a "uid" field
    $tables = array(
        'banner',
        'bannersubmission',
        'banneraccount',
    );
    foreach ($tables as $table) {
        DB_query("UPDATE {$_TABLES[$table]}
            SET uid = $destUID WHERE uid = $origUID", 1);
    }
}

 
/**
*   Update the Banner plugin
*
*   @return integer Message number to display (true = generic success msg)
*/
function plugin_upgrade_banner()
{
    global $_TABLES, $_CONF_BANR;

    $pi_name = $_CONF_BANR['pi_name'];

    // Get the current version, compare to the version in the plugin code
    $current_ver = DB_getItem($_TABLES['plugins'], 'pi_version',
        "pi_name = '$pi_name'");
    if (!$current_ver) {
        COM_errorLog("Error getting the $pi_name Plugin version",1);
        return '03';
    }

    // if we're at the current version, what are we doing here?
    if ($current_ver >= $_CONF_BANR['pi_version']) {
        COM_errorLog("Current version $current_ver is already up to date");
        return '06';
    }

    // If we're still here, import the upgrade functions.
    USES_banner_upgrade();
    $error = banner_do_upgrade($current_ver);
    if ($error) {
        COM_errorLog("Error updating the $pi_name Plugin version",1);
        return '04';
    }

    // now update the current version number.
    DB_query("UPDATE {$_TABLES['plugins']} SET
            pi_version = '{$_CONF_BANR['pi_version']}',
            pi_gl_version = '{$_CONF_BANR['gl_version']}',
            pi_homepage = '{$_CONF_BANR['pi_url']}'
        WHERE pi_name = '" . DB_escapeString($pi_name) . "'");

    if (DB_error()) {
        COM_errorLog("Error updating the $pi_name Plugin version",1);
        return '05';
    }

    COM_errorLog("Succesfully updated the $pi_name Plugin!",1);
    return true;
    // No upgrade function available yet
    COM_errorLog("Plugin upgrade function not implemented");
    return 3001;
}


/**
*   Make changes needed when the plugin is enabled or disabled.
*
*   Uses the PHP block function name instead of the block name to
*   catch user-created blocks that rely on this plugin.
*
*   @param  boolean $enable     True if we're being enabled, False otherwise
*/
function plugin_enablestatechange_banner($enabled)
{
    global $_TABLES;

    $enabled = $enabled == 1 ? 1 : 0;
    $blockfns = array('phpblock_banner_');

    foreach ($blockfns as $blockfn) {
        $sql = "UPDATE {$_TABLES['blocks']}
                SET is_enabled=$enabled
                WHERE phpblockfn like '{$blockfn}%'";
        DB_query($sql);
    }

}


/**
*   Counts the items in the submission queue.
*
*   @return integer Number of items
*/
function plugin_submissioncount_banner()
{
    global $_TABLES;

    $retval = 0;

    if (plugin_ismoderator_banner ()) {
        $retval = DB_count($_TABLES['bannersubmission']);
    }

    return $retval;
}


/**
*   Checks that the current user has plugin moderator rights.
*
*   @return boolean     Returns true if moderator
*/
function plugin_ismoderator_banner()
{
    return SEC_hasRights('banner.moderate');
}


/**
*   Returns SQL & Language texts to moderation.php
*
*   @return mixed   Plugin object or void if not allowed
*/
function plugin_itemlist_banner()
{
    global $_TABLES, $LANG_BANNER;

    if (plugin_ismoderator_banner()) {
        $plugin = new Plugin();
        $plugin->submissionlabel = $LANG_BANNER['banner_submissions'];
        //$plugin->submissionhelpfile = 'ccbannersubmission.html';
        $plugin->getsubmissionssql = 
            "SELECT 
                b.bid AS id, b.title, b.owner_id AS uid, 
                c.category, 
                camp.description
            FROM {$_TABLES['bannersubmission']} AS b
            LEFT JOIN {$_TABLES['bannercategories']} AS c
                ON c.cid = b.cid
            LEFT JOIN {$_TABLES['bannercampaigns']} as camp
                ON b.camp_id = camp.camp_id
            ORDER BY title ASC";
        $plugin->addSubmissionHeading($LANG_BANNER['title']);
        $plugin->addSubmissionHeading($LANG_BANNER['category']);
        $plugin->addSubmissionHeading($LANG_BANNER['campaign']);
        return $plugin;
    }
}


/**
*   Returns list of moderation values.
*   The array returned contains (in order): the row 'id' label, main plugin
*   table, moderation fields (comma seperated), and plugin submission table
*
*   @return array   Returns array of useful moderation values
*/
function plugin_moderationvalues_banner()
{
    global $_TABLES;

    return array ('bid',
                  $_TABLES['banner'],
                  'bid, cid, camp_id, ad_type, notes, title, 
                    publishstart, publishend, date, enabled,
                    owner_id, group_id, options, weight',
                  $_TABLES['bannersubmission']);
}


/**
*   Performs plugin exclusive work for items approved by moderation.
*
*   While moderation.php handles the actual move from bannersubmission
*   to banner tables, within the function we handle all other approval
*   relate tasks.
*
*   @param  string  $id     Banner ID
*   @return string          Any wanted HTML output
*/
function plugin_moderationapprove_banner($id)
{
    global $_TABLES, $_USER, $_GROUPS, $_CONF_BANR;

    // The bannersubmission only keeps track of the submitter's uid, but not
    // of grous and permissions. So set those to sensible defaults.
    if (isset($_GROUPS['Banner Admin'])) {
        $group_id = $_GROUPS['Banner Admin'];
    } else {
        $group_id = SEC_getFeatureGroup('banner.moderate');
    }

    $A = array();
    SEC_setDefaultPermissions($A, $_CONF_BANR['default_permissions']);

    DB_query("UPDATE {$_TABLES['banner']} 
        SET 
            group_id = '$group_id', 
            perm_owner = '{$A['perm_owner']}', 
            perm_group = '{$A['perm_group']}', 
            perm_members = '{$A['perm_members']}', 
            perm_anon = '{$A['perm_anon']}' 
        WHERE 
            bid = '$id'");

    return '';
}


/**
*   Performs plugin exclusive work for items deleted by moderation.
*
*   While moderation.php handles the actual removal from <plugin>submission
*   table, within this function we handle all other deletion
*   related tasks.
*
*   @param  string  $id     Identifying string
*   @return string          Any wanted HTML output
*/
function plugin_moderationdelete_banner($id)
{
    global $_CONF_BANR;

    $B = new Banner($id);
    $filename = $B->getOpt('filename');
    if ($filename != NULL &&
        file_exists($_CONF_BANR['img_dir'] . '/' . $filename)) {
        @unlink($_CONF_BANR['img_dir'] . '/' . $filename);
    }

    return '';
}


/**
*   Check submission form values and save if OK. Else show form again.
*
*   @param  array   $A  The banner record
*   @return string      Any wanted HTML output
*/
function plugin_savesubmission_banner($A)
{
    global $_CONF, $_CONF_BANR, $LANG12;

    USES_banner_class_banner();

    $B = new Banner();
    $retval = '';

    if ($_CONF_BANR['submissionqueue'] == 1 && 
            !SEC_hasRights('banner.submit')) {
        $B->setTable('bannersubmission');
    }

    if (!$B->Validate($A)) {
        $retval = COM_siteHeader()
                . COM_startBlock($LANG12[22], '',
                      COM_getBlockTemplate ('_msg_block', 'header'))
                . $LANG12[23]
                . COM_endBlock(COM_getBlockTemplate ('_msg_block', 'footer'))
                . plugin_submit_banner($A)
                . COM_siteFooter();
        COM_resetSpeedLimit('submit');
    } else {
        $message = $B->Save($A);
        if ($message != '') {
            $retval = COM_siteHeader()
                . BANNER_errorMessage($message)
                . plugin_submit_banner($A)
                . COM_siteFooter();
        }
    }

    if (!empty($retval)) {
        return $retval;
    } else {
        echo COM_refresh($_CONF['site_url'] . '/index.php?msg=1&plugin=' .
                $_CONF_BANR['pi_name']);
    }
}


/**
*   Shows banner submission form.
*
*   @return string  HTML for the banner submission form
*/
function plugin_submit_banner($A=array())
{
    global $_CONF_BANR, $LANG_BANNER, $_USER;

    // Make sure members can submit banners.  Anonymous users can never submit.
    if ( COM_isAnonUser() || 
        ($_CONF_BANR['usersubmit'] == 0 && !SEC_hasRights('banner.submit')) ) {
        COM_accessLog("User {$_USER['username']} tried to illegally access the banner submission screen.");
        return BANNER_errorMessage($LANG_BANNER['access_denied_msg']);
    }

    USES_banner_class_banner();

    $B = new Banner();
    $B->setAdmin(false);
    if (is_array($A)) {
        $B->setVars($A);
    }
    $retval = COM_startBlock($LANG_BANNER['submit_banner'], 
                    'submitbanner.html');
    $retval .= $B->Edit('submit');
    $retval .= COM_endBlock();

    return $retval;
}


/**
* Returns the URL of the plugin's icon
*
* @return   string      URL of the icon
*/
function plugin_geticon_banner ()
{
    return BANR_URL . '/images/banner.png';
}


/**
*   Set template variables.
*
*   A plugin should use its name as a prefix for the names of its
*   template variables, e.g. 'banner_xxx' and 'lang_banner_xxx'.
*
*   Note that there is no way to tell if a template variable is
*   actually defined.  Therefore, the impression count will be incremented
*   for each banner that this function attempts to display.  If the
*   banner_ template vars are not defined in your templates, then set
*   $_CONF_BANR['templatevars'] to false.
*
*   @param  string  $templatename   Name of template, e.g. 'header'
*   @param  ref     $template       Reference to actual template
*/
function plugin_templateSetVars_banner($templatename, &$template)
{
    global $_TABLES, $_CONF_BANR, $topic;

    USES_banner_class_banner();

    $templatenames = array();
    $options = array('limit' => '1');

    // We only operate on the 'footer' templatename since 'header' is 
    // called twice (from COM_siteHeader() and COM_siteFooter()).
    // We'll just replace both {banner_header} and {banner_footer} at once.
    if (isset($_CONF_BANR['templatevars']) && $_CONF_BANR['templatevars'] > 0) {
        if ($templatename == 'footer') {
            $templatenames = array('header','footer');
        } elseif ($templatename != 'header') {
            // Or, handle any other template names (except header)
            $templatenames = array($templatename);
        }
    } else {
        return;
    }

    if (empty($templatenames)) return;
    $options['topic'] = empty($topic) ? 'all' : $topic;

    $B = new Banner();
    foreach ($templatenames as $type) {
        $options['type'] = $type;
        $bids = Banner::GetBanner($options);
        if (is_array($bids) && !empty($bids)) {
            $B->Read($bids[0]);
            $banner = $B->BuildBanner();
            $B->updateImpressions();
            $template->set_var("banner_{$type}", $banner);
        }
    }   // foreach
}


/**
*   Automatic uninstall function for plugins
*
*   This code is automatically uninstalling the plugin.
*   It passes an array to the core code function that removes
*   tables, groups, features and php blocks from the tables.
*   Additionally, this code can perform special actions that cannot be
*   foreseen by the core code (interactions with other plugins for example)
*
*   @return   array     Elements to be removed
*/
function plugin_autouninstall_banner ()
{
    global $_CONF_BANR;

    $out = array (
        // give the name of the tables, without $_TABLES
        'tables' => array('banner', 
                    'bannersubmission',
                    'bannercategories', 
                    'bannercampaigns'),

        // give the full name of the group, as in the db
        'groups' => array('banner Admin'),

        // give the full name of the feature, as in the db
        'features' => array(
                'banner.admin',
                'banner.edit', 
                'banner.moderate', 
                'banner.submit',
        ),

        // give the full name of the block, including 'phpblock_', etc
        'php_blocks' => array(
                'phpblock_banner_topic',
                'phpblock_banner_topic_random'
        ),

        // give all vars with their name
        'vars'=> array()
    );

    if (isset($_CONF_BANR['templatevars'])) {
        $_CONF_BANR['templatevars'] = 0;
    }

    return $out;
}


/**
 *  Returns a list of banners that belong to categories associated with
 *  the current topic
 *
 *  @see    BANNER_block_banner()
 *  @return string      HTML for banner display.
 */
function phpblock_banner_topic($A=array(), $param='')
{
    global $_CONF_BANR;

    $extra = array();

    if ($_CONF_BANR['block_limit'] > 0) {
        $extra['limit'] = (int)$_CONF_BANR['block_limit'];
    }

    if (!empty($param)) {
        $extra['type'] = $param;
    } else {
        $extra['type'] = 'block';
    }

    return BANNER_block_banner($extra);
}


/**
 *  Returns a random banner that belongs to categories associated with
 *  the current topic
 *
 *  @see    BANNER_block_banner()
 *  @return string      HTML for banner display.
 */
function phpblock_banner_topic_random($A= array(), $param='')
{
    $extra = array('limit' => '1');
 
    if (!empty($param)) {
        $extra['type'] = $param;
    } else {
        $extra['type'] = 'block';
    }

    return BANNER_block_banner($extra);
}


/**
 *  Get additional Javascript into the site header.
 *
 *  @return array List of paths to Javascript files
 */
function plugin_getheaderjs_banner()
{
    $path = BANR_PI_PATH . '/js';

    $files = array(
        $path . '/toggleEnabled.js',
    );

	return $files;
}


/**
 *  Get additional CSS into the site header.
 *
 *  @return array List of paths to Javascript files
 */
function plugin_getheadercss_banner() {
    global $_CONF, $_CONF_BANR; 

    $styles = array();
    $files = array(
        'style.css', 
    );

    $path = "{$_CONF['path']}/plugins/{$_CONF_BANR['pi_name']}/css";
    foreach ($files as $file) {
    	$styles[] = $path . '/' . $file;
    }

    $path .= '/custom';
    foreach ($files as $file) {
        if (file_exists($path . '/' . $file)) {
        	$styles[] = $path . '/' . $file;
        }
    }

	return $styles;
}


/**
*   Get the user menu option for this plugin.
*
*   @return array   Array of menu items.
*/
function plugin_getuseroption_banner()
{
    global $LANG_BANNER, $_CONF, $_CONF_BANR, $_USER, $_TABLES;

    $uid = (int)$_USER['uid'];
    if ($uid < 2) return '';

    $banner_cnt = (int)DB_getItem($_TABLES['banner'], 'COUNT(*)',
                    '1=1' . COM_getPermSQL('AND', 0, 3));
    if ($banner_cnt < 1) return '';
    /*$camp_cnt = (int)DB_getItem($_TABLES['bannercampaigns'], 
                "COUNT(*)", "owner_id=$uid");
    if ($camp_cnt < 1) return '';*/

    $menu = array($LANG_BANNER['banners'],
            BANR_URL . '/index.php?mode=banners', 
            $camp_cnt);
    return $menu;

}


/**
*   Get the URL to the configuration help file
*
*   @param  string  $doclang    Document language
*   @return string              URL to document
*/
function plugin_getconfigelementhelp_banner($option, $doclang='english')
{
    $url = BANNER_docURL('config.html', $doclang);

    /*global $_CONF, $_CONF_BANR;

    $doc_path = $_CONF['path_html'] . '/' . $_CONF_BANR['pi_name'] . '/docs/';
    if (file_exists("{$doc_path}config_{$doclang}.thtml")) {
        $file  = "config_{$doclang}.thtml";
    } else {
        $file = 'config_english.html';
    }*/

    if (!empty($option)) {
        $url .= '#desc_' . $option;
    }

    //return array(BANR_URL . '/docs/' . $file, 0);
    return array($url, 0);
}


////////////////////////////////////////////
//
//  Public Plugin-Specific Functions Follow
//
////////////////////////////////////////////


/**
*   Create a user dropdown list, optionally including or excluding
*   a specific comma-separated list of user ids.  Returns the <option> 
*   clauses only.
*
*   @param  integer $sel    ID of selected user, 0 for none.
*   @param  string  $users  Comma separated list of users to include/exclude
*   @param  string  $not    'NOT' to exclude the users in $sel, blank to include
*   @return string          HTML for options in a select list.
*/
function BANNER_UserDropdown($sel=0, $users='', $not='')
{
    global $_TABLES;

    $sel = (int)$sel;

    $sql = "SELECT uid,username
            FROM {$_TABLES['users']} 
            WHERE uid <> 1 ";
    if ($users != '') {
        $not = $not == '' ? '' : 'NOT ';
        $sql .= "AND uid $not IN (" . 
                DB_escapeString($users). ") ";
    }
    $sql .= " ORDER BY username ASC";
    $result = DB_query($sql);
    if (!$result)
        return '';

    $retval = '';
    while ($row = DB_fetcharray($result)) {
        $selected = $row['uid'] == $sel ? ' selected' : '';
        $retval .= "<option value=\"{$row['uid']}\" $selected>{$row['username']}</option>\n";
    }
    return $retval;

}


/**
 *  Display a formatted error message
 *
 *  @param string $msg Error message to display
 *  @param string $type Type of message, used for style and header
 *  @param string $header Optional header text.
 *  @return string Formatted error message
 */
function BANNER_errorMessage($msg, $type = '', $header = '')
{
    global $LANG_BANNER;

    // If no header text specified, we'll put in a default.
    // Also validate the message type in this switch.
    switch (strtolower($type)) {
    case 'info':
        $class = 'info';
        if ($header == '') $header = $LANG_BANNER['info'];
        break;

    case 'note':
        $class = 'note';
        if ($header == '') $header = $LANG_BANNER['warning'];
        break;

    case 'alert':
    default:
        $class = 'alert';
        if ($header == '') $header = $LANG_BANNER['alert'];
        break;
    }

    return "<span class=\"$class\">"
            . COM_startBlock($header)
            . $msg
            . COM_endBlock()
            . '</span>';

}


/**
*   Backend function for phpblock functions.
*   Returns one or more banners which fit the request.
*
*   @param  string  $extra  Optional extra SQL.
*   @return string          HTML for banner display
*/
function BANNER_block_banner($extra='')
{
    global $_CONF, $_TABLES, $LANG_BANNER, $topic;

    USES_banner_class_banner();

    $retval = '';
    $params = array();

    if ($extra == '')
        $params[] = array('type' => 'block');
    else
        $params = $extra;

    if (!empty($topic)) {
        $tid = DB_escapeString($topic);
    } else {
        $tid = 'all';
    }
    $params['tid'] = $tid;

    // Get the banner IDs that fit the requirements.  Could be an array or a
    // single value, so convert into an array
    $bids = Banner::GetBanner($params);

    if (is_array($bids) && !empty($bids)) {
        $retval .= "<div class=\"aligncenter\">\n";
        $B = new Banner();
        foreach ($bids as $bid) {
            $B->Read($bid);
            $retval .= $B->BuildBanner() . '<br' . XHTML . '>';
            $B->updateImpressions();
        }
        $retval .= "</div>\n";
    }

    return $retval;

}


/**
*   Return the url to a documentation file.
*   Looks for the file under the user's language directory, or returns
*   the English version by default.
*   NOTE: Does not check whether the English version exists.
*
*   @param  string  $file   Filename only, e.g. 'config.thtml'
*   @param  string  $lang   Language string.  Optional.
*   @return string          Full URL to the file.
*/
function BANNER_docURL($file, $lang='')
{
    global $_CONF, $_CONF_BANR;

    if ($lang == '') $lang = $_CONF['language'];

    if (@file_exists($_CONF['path_html'] . '/' . $_CONF_BANR['pi_name'] .
            '/docs/' . $lang . '/' . $file)) {
        return BANR_URL . '/docs/' . $lang . '/' . $file;
    } else {
        return BANR_URL . '/docs/english/' . $file;
    }
}


/**
 *  Log activity to a plugin-specific log file
 *
 *  @param string $logentry Text to log
 */
function BANNER_auditLog($logentry = '')
{
    global $_CONF, $_USER, $LANG01, $_CONF_BANR;

    if ($logentry == '')
        return '';

    // A little sanitizing
    $logentry = str_replace(
            array('<?', '?>'), 
            array('(@', '@)'),
            $logentry);

    $timestamp = strftime( '%c' );
    $logfile = $_CONF['path_log'] . $_CONF_BANR['pi_name'] . '.log';

    // Can't open the log file?  Return an error
    if (!$file = fopen($logfile, 'a')) {
        return $LANG01[33] . $logfile . ' (' . $timestamp . ')<br' . XHTML . '>' . LB;
    }

    // Get the user name if it's not anonymous
    if (!COM_isAnonUser()) {
        $byuser = $_USER['uid'] . '-'. 
                COM_getDisplayName($_USER['uid'], 
                    $_USER['username'], $_USER['fullname']);
    } else {
        $byuser = 'anon';
    }
    $byuser .= '@' . $_SERVER['REMOTE_ADDR'];

    // Write the log entry to the file
    fputs($file, "$timestamp ($byuser) - $logentry\n");

}


?>
<?php
/**
 * Class to handle banner images.
 *
 * @author      Lee Garner <lee@leegarner.com>
 * @copyright   Copyright (c) 2009-2017 Lee Garner <lee@leegarner.com>
 * @package     banner
 * @version     v0.2.1
 * @license     http://opensource.org/licenses/gpl-2.0.php
 *              GNU Public License v2 or later
 * @filesource
 */
namespace Banner;

/**
 * Image-handling class.
 * @package banner
 */
class Image extends \upload
{
    /** Path to actual image (without filename).
     * @var string */
    var $pathImage;

    /** ID of the current banner.
     * @var string */
    var $bid;

    /** Array of the names of successfully uploaded files.
     * @var array */
    var $goodfiles = array();

    /** File extension
     * @var string */
    var $ext;

    /**
     * Constructor.
     *
     * @param   string   $bid        Banner ID
     * @param   string   $varname    Form variable name, optional
     */
    public function __construct($bid, $varname='bannerimage')
    {
        global $_CONF_BANR, $_CONF;

        $this->setContinueOnError(true);
        //$this->setLogFile($_CONF['path_log'] 'error.log');
        //$this->setDebug(true);
        parent::__construct();       // call the parent constructor

        // Before anything else, check the upload directory
        if (!$this->setPath($_CONF_BANR['img_dir'])) {
            //$this->_addError('Failed to set path to '.$_CONF_BANR['img_dir']);
            return;
        }
        $this->bid = $bid;
        $this->pathImage = $_CONF_BANR['img_dir'];
        $this->setAllowedMimeTypes(array(
                'image/pjpeg' => '.jpg,.jpeg',
                'image/jpeg'  => '.jpg,.jpeg',
                'image/png'   => '.png',
                'image/x-png' => '.png',
                'image/gif'   => '.gif',
        ));
        $this->setMaxFileSize($_CONF['max_image_size']);
        $this->setMaxDimensions(
                $_CONF_BANR['img_max_width'],
                $_CONF_BANR['img_max_height']
        );
        $this->setAutomaticResize(false);
        $this->setFieldName($varname);

        switch ($_FILES[$varname]['type']) {
        case 'image/pjpeg':
        case 'image/jpeg':
            $this->ext = 'jpg';
            break;
        case 'image/png':
        case 'image/x-png':
            $this->ext = 'png';
            break;
        case 'image/gif':
            $this->ext = 'gif';
            break;
        }

        $this->filename = $this->bid . '.' . $this->ext;
        $this->setFileNames($this->filename);
    }


    /**
    *   Get the filename for the current image
    *
    *   @return string  Image filenmae
    */
    public function getFilename()
    {
        return $this->filename;
    }


    /**
     * Calculate the new dimensions needed to keep the image within
     * the provided width & height while preserving the aspect ratio.
     *
     * @deprecated Using LGLIB now
     * @param   integer $s_width    Source width, in pixels
     * @param   integer $s_height   Source height, in pixels
     * @param   integer $maxdim     Maximum width/height
     * @return  array  $newwidth, $newheight
     */
    public function XreDim($s_width, $s_height, $maxdim=0)
    {
        // get both sizefactors that would resize one dimension correctly
        if ($maxdim > 0 && $s_width > $maxdim)
            $sizefactor_w = (double) ($maxdim / $s_width);
        else
            $sizefactor_w = 1;

        if ($maxdim > 0 && $s_height > $maxdim)
            $sizefactor_h = (double) ($maxdim / $s_height);
        else
            $sizefactor_h = 1;

        // Use the smaller factor to stay within the parameters
        $sizefactor = min($sizefactor_w, $sizefactor_h);

        $newwidth = (int)($s_width * $sizefactor);
        $newheight = (int)($s_height * $sizefactor);

        return array($newwidth, $newheight);
    }

}   // class Image

?>

<?php
/**
 * @license   http://www.gnu.org/licenses/gpl.html GPL Version 3
 * @author    Volker Theile <volker.theile@openmediavault.org>
 * @author    OpenMediaVault Plugin Developers <plugins@omv-extras.org>
 * @copyright Copyright (c) 2009-2013 Volker Theile
 * @copyright Copyright (c) 2013-2017 OpenMediaVault Plugin Developers
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

class OMVRpcServiceResetPerms extends \OMV\Rpc\ServiceAbstract
{
    public function getName()
    {
        return "ResetPerms";
    }

    public function initialize()
    {
        $this->registerMethod("getSettings");
        $this->registerMethod("setSettings");
        $this->registerMethod("doResetPerms");

        $this->registerMethod("getSharedFolderInUseList");
    }

    public function getSettings($params, $context)
    {
        // Validate the RPC caller context.
        $this->validateMethodContext($context, ["role" => OMV_ROLE_ADMINISTRATOR]);
        // Get the configuration object.
        $db = \OMV\Config\Database::getInstance();
        $object = $db->get("conf.system.resetperms");
        return $object->getAssoc();
    }

    public function setSettings($params, $context)
    {
        // Validate the RPC caller context.
        $this->validateMethodContext($context, ["role" => OMV_ROLE_ADMINISTRATOR]);
        // Validate the parameters of the RPC service method.
        $this->validateMethodParams($params, "rpc.resetperms.setsettings");
        // Get the existing configuration object.
        $db = \OMV\Config\Database::getInstance();
        $object = $db->get("conf.system.resetperms");
        switch ($params['mode']) {
            case "700": $params['fileperms'] = "600"; break;
            case "750": $params['fileperms'] = "640"; break;
            case "770": $params['fileperms'] = "660"; break;
            case "755": $params['fileperms'] = "644"; break;
            case "775": $params['fileperms'] = "664"; break;
            case "777": $params['fileperms'] = "666"; break;
            default:    $params['fileperms'] = "644";
        }
        $params['dirperms'] = sprintf("2%s", $params['mode']);
        $object->setAssoc($params);
        $db->set($object);
        // Return the configuration object.
        return $object->getAssoc();
    }

    public function doResetPerms($params, $context)
    {
        // Validate the RPC caller context.
        $this->validateMethodContext($context, ["role" => OMV_ROLE_ADMINISTRATOR]);
        // Get the configuration object.
        $db = \OMV\Config\Database::getInstance();
        $object = $db->get("conf.system.resetperms");
        // Check if shared folder is set to none
        if (is_null($object->get('sharedfolderref'))) {
            throw new \OMV\Exception(gettext("Please select a shared folder."));
        }
        // Create a background process.
        $bgStatusFilename = $this->createBgProcStatus();
        $pid = $this->fork();
        if ($pid > 0) { // Parent process.
            $this->initializeBgProcStatus($bgStatusFilename, $pid);
            return $bgStatusFilename;
        }
        // Child process.
        try {
            $bgOutputFilename = $this->createBgProcOutput();
            $this->updateBgProcStatus($bgStatusFilename, 'outputfilename', $bgOutputFilename);
            // Execute command.
            $cmd = 'export LANG=C; omv-mkconf resetperms 2>&1';
            $this->exec($cmd, $output, $bgOutputFilename);
            // Finalize the background process.
            $this->finalizeBgProcStatus($bgStatusFilename, $output);
            exit(0);
        } catch(\Exception $e) {
            $this->finalizeBgProcStatus($bgStatusFilename, '', $e);
            exit(1);
        }
    }

    public function getSharedFolderInUseList($params, $context)
    {
        // Validate the RPC caller context.
        $this->validateMethodContext($context, ["role" => OMV_ROLE_ADMINISTRATOR]);
        // Validate the parameters of the RPC service method.
        $this->validateMethodParams($params, 'rpc.common.getlist');
        // Get all configured shared folder configuration objects.
        $db = \OMV\Config\Database::getInstance();
        $objects = $db->get("conf.system.sharedfolder");
        // Add additional informations.
        $objectsAssoc = [];
        foreach ($objects as $objectk => &$objectv) {
            $output = [];
            $cmd = sprintf("omv-showsharedfolder %s", $objectv->get('name'));
            exec($cmd, $output);
            $objectv->add('inuse', 'string', implode('<br/>', $output));
            $objectsAssoc[] = $objectv->getAssoc();
        }
        // Filter the result.
        return $this->applyFilter($objectsAssoc, $params['start'], $params['limit'],
            $params['sortfield'], $params['sortdir']);
    }
}

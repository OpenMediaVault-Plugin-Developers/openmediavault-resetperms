<?php
/**
 * @license   http://www.gnu.org/licenses/gpl.html GPL Version 3
 * @author    Volker Theile <volker.theile@openmediavault.org>
 * @author    OpenMediaVault Plugin Developers <plugins@omv-extras.org>
 * @copyright Copyright (c) 2009-2013 Volker Theile
 * @copyright Copyright (c) 2013-2022 OpenMediaVault Plugin Developers
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
        $this->registerMethod("doResetPerms");
        $this->registerMethod("getSharedFolderInUseList");
    }

    public function doResetPerms($params, $context)
    {
        // Validate the RPC caller context.
        $this->validateMethodContext($context, ["role" => OMV_ROLE_ADMINISTRATOR]);
        $cmdArgs = [];
        $cmdArgs[] = 'omv-resetperms';
        $cmdArgs[] = sprintf('"%s"', $params['path']);
        $cmdArgs[] = $params['mode'];
        $cmdArgs[] = $params['clearacl'];
        $cmd = new \OMV\System\Process($cmdArgs);
        $cmd->execute($output, $exitStatus);
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
            $sfref = $objectv->get("uuid");
            $sfpath = \OMV\Rpc\Rpc::call("ShareMgmt", "getPath", ["uuid" => $sfref], $context);
            $sfpath = rtrim($sfpath, '/');
            $objectv->add('path', 'string', $sfpath);
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

# Copyright 2020 Huawei Technologies Co., Ltd
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
# ============================================================================
"""
Image classifiation.
"""
import math
import mindspore.nn as nn
from mindspore.common import initializer as init
import src.backbone as backbones
import src.head as heads
from src.utils.var_init import default_recurisive_init, KaimingNormal


class ImageClassificationNetwork(nn.Cell):
    """
    architecture of image classification network.

    Args:
    Returns:
        Tensor, output tensor.
    """
    def __init__(self, backbone, head):
        super(ImageClassificationNetwork, self).__init__()
        self.backbone = backbone
        self.head = head

    def construct(self, x):
        x = self.backbone(x)
        x = self.head(x)
        return x

class Resnet(ImageClassificationNetwork):
    """
    Resnet architecture.
    Args:
        backbone_name (string): backbone.
        num_classes (int): number of classes.
    Returns:
        Resnet.
    """
    def __init__(self, backbone_name, num_classes, platform="Ascend"):
        self.backbone_name = backbone_name
        backbone = backbones.__dict__[self.backbone_name](platform=platform)
        out_channels = backbone.get_out_channels()
        head = heads.CommonHead(num_classes=num_classes, out_channels=out_channels)
        super(Resnet, self).__init__(backbone, head)

        default_recurisive_init(self)

        for cell in self.cells_and_names():
            if isinstance(cell, nn.Conv2d):
                cell.weight.default_input = init.initializer(
                    KaimingNormal(a=math.sqrt(5), mode='fan_out', nonlinearity='relu'),
                    cell.weight.default_input.shape, cell.weight.default_input.dtype).to_tensor()
            elif isinstance(cell, nn.BatchNorm2d):
                cell.gamma.default_input = init.initializer('ones', cell.gamma.default_input.shape).to_tensor()
                cell.beta.default_input = init.initializer('zeros', cell.beta.default_input.shape).to_tensor()

        # Zero-initialize the last BN in each residual branch,
        # so that the residual branch starts with zeros, and each residual block behaves like an identity.
        # This improves the model by 0.2~0.3% according to https://arxiv.org/abs/1706.02677
        for cell in self.cells_and_names():
            if isinstance(cell, backbones.resnet.Bottleneck):
                cell.bn3.gamma.default_input = init.initializer('zeros', cell.bn3.gamma.default_input.shape).to_tensor()
            elif isinstance(cell, backbones.resnet.BasicBlock):
                cell.bn2.gamma.default_input = init.initializer('zeros', cell.bn2.gamma.default_input.shape).to_tensor()



def get_network(backbone_name, num_classes, platform="Ascend"):
    if backbone_name in ['resnext50']:
        return Resnet(backbone_name, num_classes, platform)
    return None

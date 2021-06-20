#pragma once

#ifndef NAZARA_SHADERNODES_BOOLVALUE_HPP
#define NAZARA_SHADERNODES_BOOLVALUE_HPP

#include <QtGui/QImage>
#include <QtWidgets/QDoubleSpinBox>
#include <QtWidgets/QFormLayout>
#include <QtWidgets/QLabel>
#include <ShaderNode/DataModels/ShaderNode.hpp>
#include <ShaderNode/DataTypes/FloatData.hpp>
#include <array>

class BoolValue : public ShaderNode
{
	public:
		BoolValue(ShaderGraph& graph);
		~BoolValue() = default;

		Nz::ShaderAst::NodePtr BuildNode(Nz::ShaderAst::ExpressionPtr* expressions, std::size_t count, std::size_t outputIndex) const override;
		void BuildNodeEdition(QFormLayout* layout) override;

		QString caption() const override;
		QString name() const override;

		QtNodes::NodeDataType dataType(QtNodes::PortType portType, QtNodes::PortIndex portIndex) const override;

		unsigned int nPorts(QtNodes::PortType portType) const override;

		std::shared_ptr<QtNodes::NodeData> outData(QtNodes::PortIndex port) override;

		QString portCaption(QtNodes::PortType portType, QtNodes::PortIndex portIndex) const override;
		bool portCaptionVisible(QtNodes::PortType portType, QtNodes::PortIndex portIndex) const override;

	private:
		bool ComputePreview(QPixmap& pixmap) override;
		QColor ToColor() const;

		void restore(const QJsonObject& data) override;
		QJsonObject save() const override;

		bool m_value;
};

#include <ShaderNode/DataModels/BoolValue.inl>

#endif

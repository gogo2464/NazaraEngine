// Copyright (C) 2020 Jérôme Leclercq
// This file is part of the "Nazara Engine - Shader generator"
// For conditions of distribution and use, see copyright notice in Config.hpp

#pragma once

#ifndef NAZARA_SHADER_AST_EXPRESSIONTYPE_HPP
#define NAZARA_SHADER_AST_EXPRESSIONTYPE_HPP

#include <Nazara/Prerequisites.hpp>
#include <Nazara/Utility/Enums.hpp>
#include <Nazara/Shader/ShaderEnums.hpp>
#include <Nazara/Shader/Ast/Attribute.hpp>
#include <string>
#include <variant>
#include <vector>

namespace Nz::ShaderAst
{
	struct IdentifierType //< Alias or struct
	{
		std::string name;

		inline bool operator==(const IdentifierType& rhs) const;
		inline bool operator!=(const IdentifierType& rhs) const;
	};

	struct MatrixType
	{
		std::size_t columnCount;
		std::size_t rowCount;
		PrimitiveType type;

		inline bool operator==(const MatrixType& rhs) const;
		inline bool operator!=(const MatrixType& rhs) const;
	};

	struct NoType
	{
		inline bool operator==(const NoType& rhs) const;
		inline bool operator!=(const NoType& rhs) const;
	};

	struct SamplerType
	{
		ImageType dim;
		PrimitiveType sampledType;

		inline bool operator==(const SamplerType& rhs) const;
		inline bool operator!=(const SamplerType& rhs) const;
	};

	struct UniformType
	{
		IdentifierType containedType;

		inline bool operator==(const UniformType& rhs) const;
		inline bool operator!=(const UniformType& rhs) const;
	};

	struct VectorType
	{
		std::size_t componentCount;
		PrimitiveType type;

		inline bool operator==(const VectorType& rhs) const;
		inline bool operator!=(const VectorType& rhs) const;
	};

	using ExpressionType = std::variant<NoType, IdentifierType, PrimitiveType, MatrixType, SamplerType, UniformType, VectorType>;

	struct StructDescription
	{
		struct StructMember
		{
			std::string name;
			std::vector<Attribute> attributes;
			ExpressionType type;
		};

		std::string name;
		std::vector<StructMember> members;
	};

	inline bool IsIdentifierType(const ExpressionType& type);
	inline bool IsMatrixType(const ExpressionType& type);
	inline bool IsNoType(const ExpressionType& type);
	inline bool IsPrimitiveType(const ExpressionType& type);
	inline bool IsSamplerType(const ExpressionType& type);
	inline bool IsUniformType(const ExpressionType& type);
	inline bool IsVectorType(const ExpressionType& type);
}

#include <Nazara/Shader/Ast/ExpressionType.inl>

#endif

#pragma once

enum class ReaderActionType
{
	ResolvePtr,
	CustomResolver,
	InsertCollectionItem,
	CallMutator,
};
